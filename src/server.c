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
 * dentro de mutex_lock()/mutex_unlock() (la seccion critica). El mismo mutex
 * protege el roster (SessionRoster): quien tiene cada id de jugador.
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
    GameState game;       /* recurso compartido protegido por la seccion critica */
    SessionRoster roster; /* idem: quien tiene cada id de jugador */
    Mutex mutex;           /* mutex que protege 'game' y 'roster' */
    int running;
    SOCKET serverSocket;
    Thread physics;
    Thread accept;
} ServerState;

static ServerState server;

/* Datos de una conexion recien aceptada, pasados al hilo del cliente. */
typedef struct
{
    SOCKET sock;
    char ip[64];
} ClientConn;

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

    out->roster = server.roster; /* el roster viaja "gratis" en cada snapshot */
}

static int countSlotUsed(void)
{
    int n = 0;
    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
        if (server.roster.slotUsed[i])
            n++;
    return n;
}

/*
 * Asigna el id de jugador (1..4) para una conexion nueva. Debe llamarse con
 * server.mutex ya tomado. Reglas, en orden:
 *   1) preferredId valido y libre  -> se lo devolvemos tal cual (rejoin
 *      explicito: el propio proceso recuerda su id, o el host se reconecta a
 *      si mismo por 127.0.0.1 tras la migracion).
 *   2) Un slot ya registrado con la MISMA IP, actualmente libre -> recupera
 *      su id anterior (rejoin tras cerrar y reabrir el cliente).
 *   3) El primer slot libre disponible.
 * Devuelve 0 si no hay cupo (sala llena).
 */
static int assignPlayerId(int preferredId, const char *remoteIp)
{
    if (preferredId >= 1 && preferredId <= MAX_PACKET_PLAYERS &&
        !server.game.players[preferredId - 1].active)
        return preferredId;

    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
    {
        if (server.roster.slotUsed[i] && !server.game.players[i].active &&
            strncmp(server.roster.ips[i], remoteIp, sizeof(server.roster.ips[i])) == 0)
            return i + 1;
    }

    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
        if (!server.game.players[i].active)
            return i + 1;

    return 0;
}

/*
 * HILO DE FISICA.
 * Cada tick entra a la seccion critica para actualizar la pelota, resolver
 * colisiones y goles sobre el estado compartido, y vuelve a salir.
 */
static THREAD_RET physicsThread(void *arg)
{
    (void)arg;

    int tick = 0;        /* para imprimir un latido cada segundo */
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
 * Primero hace el handshake (JoinRequest/JoinResponse) para asignar el id de
 * jugador; despues recibe en bucle la posicion del jugador (conexion
 * persistente), la escribe en el estado compartido bajo el mutex, toma una
 * "foto" del estado (con el roster) y se la devuelve al cliente.
 */
static THREAD_RET clientThread(void *arg)
{
    ClientConn *conn = (ClientConn *)arg;
    SOCKET clientSocket = conn->sock;
    char remoteIp[64];
    strncpy(remoteIp, conn->ip, sizeof(remoteIp) - 1);
    remoteIp[sizeof(remoteIp) - 1] = '\0';
    free(conn);

    JoinRequestPacket joinReq;
    int joinBytes = recv(clientSocket, (char *)&joinReq, sizeof(joinReq), 0);

    if (joinBytes != (int)sizeof(joinReq) || joinReq.type != NET_MSG_JOIN_REQUEST)
    {
        closesocket(clientSocket);
        return 0;
    }

    mutex_lock(&server.mutex); /* ---- entra a seccion critica ---- */

    int assignedId = assignPlayerId(joinReq.preferredId, remoteIp);

    if (assignedId != 0)
    {
        int wasNewSlot = !server.roster.slotUsed[assignedId - 1];

        server.roster.slotUsed[assignedId - 1] = 1;
        strncpy(server.roster.ips[assignedId - 1], remoteIp,
                sizeof(server.roster.ips[assignedId - 1]) - 1);
        server.roster.ips[assignedId - 1][sizeof(server.roster.ips[assignedId - 1]) - 1] = '\0';

        if (wasNewSlot)
            server.roster.registeredCount = countSlotUsed();

        if (server.roster.currentHostId == 0)
            server.roster.currentHostId = assignedId; /* el primero en unirse es el host */

        server.game.players[assignedId - 1].active = 1;
        server.roster.slotActive[assignedId - 1] = 1;
    }

    JoinResponsePacket joinResp;
    joinResp.type = NET_MSG_JOIN_RESPONSE;
    joinResp.assignedId = assignedId;
    joinResp.roster = server.roster;

    mutex_unlock(&server.mutex); /* ---- sale de seccion critica ---- */

    if (assignedId == 0)
    {
        printf("[SERVIDOR] Conexion rechazada desde %s: sala llena\n", remoteIp);
        fflush(stdout);
        send(clientSocket, (char *)&joinResp, sizeof(joinResp), 0);
        closesocket(clientSocket);
        return 0;
    }

    if (send(clientSocket, (char *)&joinResp, sizeof(joinResp), 0) <= 0)
    {
        mutex_lock(&server.mutex);
        server.game.players[assignedId - 1].active = 0;
        server.roster.slotActive[assignedId - 1] = 0;
        mutex_unlock(&server.mutex);
        closesocket(clientSocket);
        return 0;
    }

    int myIndex = assignedId - 1;

    printf("[SERVIDOR] Jugador %d conectado desde %s\n", assignedId, remoteIp);
    fflush(stdout);

    PlayerPacket packet;
    GamePacket snapshot;
    int recvCount = 0;

    while (server.running)
    {
        int bytes = recv(clientSocket, (char *)&packet, sizeof(PlayerPacket), 0);

        if (bytes <= 0)
            break; /* el cliente se desconecto */

        if (bytes != (int)sizeof(PlayerPacket))
            continue;

        recvCount++;
        if (recvCount == 1 || recvCount % 150 == 0) /* sin saturar la consola */
        {
            printf("[HILO CLIENTE] Jugador %d envia posicion -> entra a seccion critica\n",
                   assignedId);
            fflush(stdout);
        }

        mutex_lock(&server.mutex); /* ---- entra a seccion critica ---- */

        /* El id de este socket lo decidio el handshake, no lo que mande el
         * cliente en cada paquete (packet.playerId se ignora a proposito). */
        Player *p = &server.game.players[myIndex];
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

    /* Al desconectarse, marcamos al jugador como inactivo (bajo el mutex).
     * El roster (slotUsed/ips) NO se borra: asi, si la misma IP reconecta,
     * recupera su id. slotActive si se borra: ya no esta conectado AHORA,
     * aunque siga "reservado" para rejoin. */
    mutex_lock(&server.mutex);
    server.game.players[myIndex].active = 0;
    server.roster.slotActive[myIndex] = 0;
    mutex_unlock(&server.mutex);

    printf("[SERVIDOR] Jugador %d desconectado\n", assignedId);
    fflush(stdout);

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

        ClientConn *conn = (ClientConn *)malloc(sizeof(ClientConn));
        conn->sock = clientSocket;
        strncpy(conn->ip, inet_ntoa(clientAddr.sin_addr), sizeof(conn->ip) - 1);
        conn->ip[sizeof(conn->ip) - 1] = '\0';

        printf("[SERVIDOR] Nueva conexion desde %s\n", conn->ip);
        fflush(stdout);

        Thread t;
        if (thread_create(&t, clientThread, conn))
            thread_detach(t); /* no esperamos a cada cliente con join */
        else
        {
            free(conn);
            closesocket(clientSocket);
        }
    }

    return 0;
}

int serverStart(void)
{
    return serverStartWithRoster(NULL);
}

int serverStartWithRoster(const SessionRoster *roster)
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
     * hasta que cada cliente se conecte (incluso si el roster trae IPs
     * precargadas de una sesion anterior: el partido siempre arranca de
     * cero con initGame(), solo se recuerdan los ids/IPs). */
    initGame(&server.game);
    for (int i = 0; i < MAX_PLAYERS; i++)
        server.game.players[i].active = 0;

    /* Si viene de una migracion, el roster ya trae slotActive corregido (el
     * host caido marcado inactivo, ver computeNextHostAmongActive en
     * network.c) y currentHostId = este equipo. Se copia tal cual: cada
     * jugador reconfirma su slotActive al rehacer el handshake. */
    if (roster != NULL)
        server.roster = *roster;
    else
        memset(&server.roster, 0, sizeof(server.roster));

    mutex_init(&server.mutex);
    server.running = 1;

    server.serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.serverSocket == INVALID_SOCKET)
    {
        printf("[SERVIDOR] Error creando el socket\n");
        return 0;
    }

    /* SO_REUSEADDR: al migrar, el puerto del host anterior puede quedar en
     * TIME_WAIT unos segundos; sin esto el bind() de abajo fallaria. */
    int reuse = 1;
    setsockopt(server.serverSocket, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&reuse, sizeof(reuse));

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
