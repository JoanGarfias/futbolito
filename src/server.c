/*
 * SERVIDOR AUTORITATIVO MULTIHILO de Futbolito (modulo reutilizable).
 *
 * Arquitectura Cliente/Servidor sobre sockets TCP, con paralelismo
 * multihilo sincronizado por una SECCION CRITICA con mutex:
 *
 *   - 1 hilo que acepta conexiones (acceptThread)
 *   - 1 hilo por cada cliente conectado (clientThread)
 *   - 1 hilo de fisica ~60 fps (physicsThread)
 *
 * Todos esos hilos LEEN y ESCRIBEN sobre el mismo GameState compartido. Sin el
 * mutex habria condicion de carrera; por eso cada acceso al estado se hace
 * dentro de mutex_lock()/mutex_unlock() (la seccion critica).
 *
 * Compila igual en Windows (Winsock + CreateThread + CRITICAL_SECTION) y en
 * Linux (sockets BSD + pthread_create + pthread_mutex_t) gracias a threads.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#define socket_shutdown(sock) shutdown((sock), SD_BOTH)
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#define socket_shutdown(sock) shutdown((sock), SHUT_RDWR)
#endif

#include "../include/threads.h"
#include "../include/server.h"
#include "../include/game.h"
#include "../include/physics.h"
#include "../include/network_packet.h"

#define PORT 5000
#define PHYSICS_TICK_MS 16 /* ~60 fps */

/* Estado compartido entre TODOS los hilos del servidor. */
typedef struct
{
    GameState game; /* recurso compartido protegido por la seccion critica */
    Mutex mutex;    /* mutex que protege 'game' */
    int running;
    SOCKET serverSocket;
    Thread physics;
    Thread accept;
} ServerState;

static ServerState server;

/* Convierte el GameState interno al paquete que viaja por la red. */
static void buildSnapshot(GamePacket *out, const GameState *g)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        out->players[i].playerId = g->players[i].id;
        out->players[i].x = g->players[i].x;
        out->players[i].y = g->players[i].y;
        out->players[i].dirX = g->players[i].dirX;
        out->players[i].dirY = g->players[i].dirY;
        out->players[i].active = g->players[i].active;
        out->score[i] = g->score[i];
    }

    out->winner = g->winner;

    out->ball.x = g->ball.x;
    out->ball.y = g->ball.y;
    out->ball.vx = g->ball.vx;
    out->ball.vy = g->ball.vy;
    out->ball.size = g->ball.size;
    out->ball.lastPlayerTouched = g->ball.lastPlayerTouched;
}

/*
 * HILO DE FISICA.
 * Cada tick entra a la seccion critica para actualizar la pelota, resolver
 * colisiones y goles sobre el estado compartido, y vuelve a salir.
 */
static THREAD_RET physicsThread(void *arg)
{
    (void)arg;

    int tick = 0;       /* para imprimir un latido cada segundo */
    int winnerTicks = 0; /* cuenta el tiempo tras ganar, para reiniciar */

    while (server.running)
    {
        int activeCount = 0;
        float bx = 0, by = 0;

        mutex_lock(&server.mutex); /* ---- entra a seccion critica ---- */

        updatePhysics(&server.game);

        if (server.game.winner != -1)
        {
            winnerTicks++;
            if (winnerTicks > 180) /* ~3 segundos: nueva partida */
            {
                for (int i = 0; i < MAX_PLAYERS; i++)
                    server.game.score[i] = 0;
                server.game.winner = -1;
                resetBall(&server.game);
                winnerTicks = 0;
                printf("[SERVIDOR] Marcador reiniciado, nueva partida\n");
                fflush(stdout);
            }
        }
        else
        {
            winnerTicks = 0;
        }

        for (int i = 0; i < MAX_PLAYERS; i++)
            if (server.game.players[i].active)
                activeCount++;
        bx = server.game.ball.x;
        by = server.game.ball.y;

        mutex_unlock(&server.mutex); /* ---- sale de seccion critica ---- */

        tick++;
        if (tick % 60 == 0) /* una vez por segundo */
        {
            printf("[HILO FISICA] seccion critica OK | pelota=(%.0f,%.0f) | jugadores activos=%d\n",
                   bx, by, activeCount);
            fflush(stdout);
        }

        thread_sleep_ms(PHYSICS_TICK_MS);
    }

    return 0;
}

/*
 * HILO POR CLIENTE.
 * Recibe en bucle la posicion del jugador (conexion persistente), la escribe
 * en el estado compartido bajo el mutex, toma una "foto" del estado y se la
 * devuelve al cliente.
 */
static THREAD_RET clientThread(void *arg)
{
    SOCKET clientSocket = *(SOCKET *)arg;
    free(arg);

    PlayerPacket packet;
    GamePacket snapshot;
    int myIndex = -1;
    int recvCount = 0;

    while (server.running)
    {
        int bytes = recv(clientSocket, (char *)&packet, sizeof(PlayerPacket), 0);

        if (bytes <= 0)
            break; /* el cliente se desconecto */

        if (bytes != (int)sizeof(PlayerPacket))
            continue;

        int index = packet.playerId - 1;

        if (index < 0 || index >= MAX_PLAYERS)
            continue;

        myIndex = index;

        recvCount++;
        if (recvCount == 1 || recvCount % 150 == 0) /* sin saturar la consola */
        {
            printf("[HILO CLIENTE] Jugador %d envia posicion -> entra a seccion critica\n",
                   index + 1);
            fflush(stdout);
        }

        mutex_lock(&server.mutex); /* ---- entra a seccion critica ---- */

        Player *p = &server.game.players[index];
        p->prevX = packet.x;
        p->prevY = packet.y;
        p->x = packet.x;
        p->y = packet.y;
        p->dirX = packet.dirX;
        p->dirY = packet.dirY;
        p->active = 1;

        buildSnapshot(&snapshot, &server.game);

        mutex_unlock(&server.mutex); /* ---- sale de seccion critica ---- */

        if (send(clientSocket, (char *)&snapshot, sizeof(GamePacket), 0) <= 0)
            break;
    }

    /* Al desconectarse, marcamos al jugador como inactivo (bajo el mutex). */
    if (myIndex >= 0)
    {
        mutex_lock(&server.mutex);
        server.game.players[myIndex].active = 0;
        mutex_unlock(&server.mutex);

        printf("[SERVIDOR] Jugador %d desconectado\n", myIndex + 1);
        fflush(stdout);
    }

    closesocket(clientSocket);
    return 0;
}

/*
 * HILO QUE ACEPTA CONEXIONES.
 * Por cada cliente nuevo lanza un clientThread (conexion TCP persistente).
 */
static THREAD_RET acceptThread(void *arg)
{
    (void)arg;

    while (server.running)
    {
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        SOCKET clientSocket = accept(server.serverSocket,
                                     (struct sockaddr *)&clientAddr, &len);

        if (clientSocket == INVALID_SOCKET)
            continue; /* si server.running pasa a 0, accept falla y salimos */

        printf("[SERVIDOR] Nueva conexion desde %s\n",
               inet_ntoa(clientAddr.sin_addr));
        fflush(stdout);

        SOCKET *connArg = (SOCKET *)malloc(sizeof(SOCKET));
        *connArg = clientSocket;

        Thread t;
        if (thread_create(&t, clientThread, connArg))
            thread_detach(t); /* no esperamos a cada cliente con join */
        else
        {
            free(connArg);
            closesocket(clientSocket);
        }
    }

    return 0;
}

int serverStart(void)
{
    struct sockaddr_in serverAddr;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("[SERVIDOR] Error iniciando Winsock\n");
        return 0;
    }
#endif

    /* Estado inicial: campo armado pero TODOS los jugadores inactivos
     * hasta que cada cliente se conecte. */
    initGame(&server.game);
    for (int i = 0; i < MAX_PLAYERS; i++)
        server.game.players[i].active = 0;

    mutex_init(&server.mutex);
    server.running = 1;

    server.serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.serverSocket == INVALID_SOCKET)
    {
        printf("[SERVIDOR] Error creando el socket\n");
        return 0;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(server.serverSocket, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        printf("[SERVIDOR] Error en bind (puerto %d ocupado?)\n", PORT);
        closesocket(server.serverSocket);
        return 0;
    }

    listen(server.serverSocket, MAX_PLAYERS);

    printf("[SERVIDOR] Escuchando en el puerto %d (max %d jugadores)\n",
           PORT, MAX_PLAYERS);
    fflush(stdout);

    if (!thread_create(&server.physics, physicsThread, NULL))
    {
        printf("[SERVIDOR] Error creando el hilo de fisica\n");
        return 0;
    }

    if (!thread_create(&server.accept, acceptThread, NULL))
    {
        printf("[SERVIDOR] Error creando el hilo de aceptacion\n");
        return 0;
    }

    return 1;
}

void serverStop(void)
{
    server.running = 0;
    socket_shutdown(server.serverSocket);
    closesocket(server.serverSocket); /* desbloquea el accept() */

    thread_join(server.accept);
    thread_join(server.physics);

    mutex_destroy(&server.mutex);

#ifdef _WIN32
    WSACleanup();
#endif
}
