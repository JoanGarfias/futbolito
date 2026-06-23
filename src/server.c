/* Servidor autoritativo del Futbolito. Tres hilos corriendo a la vez:
 * acceptThread (acepta conexiones), un clientThread por cada jugador
 * conectado, y physicsThread (fisica a ~60 fps). Todos leen/escriben el
 * mismo GameState y el roster, por eso van protegidos con server.mutex
 * (si no, condicion de carrera). threads.h se encarga de que esto compile
 * igual en Windows y en Linux. */

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
    out->goalCount = g->goalCount;

    out->ball.x = g->ball.x;
    out->ball.y = g->ball.y;
    out->ball.vx = g->ball.vx;
    out->ball.vy = g->ball.vy;
    out->ball.size = g->ball.size;
    out->ball.lastPlayerTouched = g->ball.lastPlayerTouched;
    out->ball.kickCount = g->ball.kickCount;

    for (int i = 0; i < CHAT_HISTORY; i++)
    {
        out->chat[i].playerId = g->chatLog[i].playerId;
        strncpy(out->chat[i].text, g->chatLog[i].text, CHAT_MAX_LEN - 1);
        out->chat[i].text[CHAT_MAX_LEN - 1] = '\0';
    }

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

/* Decide que id (1..4) le toca a una conexion nueva. Llamar con server.mutex
 * ya tomado. Primero respeta el preferredId si esta libre (rejoin explicito),
 * luego intenta recuperar un slot viejo con la misma IP, y si no hay nada de
 * eso agarra el primer slot libre. 0 si ya no hay cupo. */
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

/* Cada tick entra al mutex, actualiza pelota/colisiones/goles, y sale. */
static THREAD_RET physicsThread(void *arg)
{
    (void)arg;

    int tick = 0;        /* para imprimir un latido cada segundo */
    int winnerTicks = 0; /* cuenta el tiempo tras ganar, para reiniciar */

    while (server.running)
    {
        int activeCount = 0;
        float bx = 0, by = 0;

        mutex_lock(&server.mutex);

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

        mutex_unlock(&server.mutex);

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

/* Uno de estos por cliente. Primero el handshake para asignarle el id,
 * despues queda en bucle: recibe su posicion, la guarda bajo el mutex,
 * arma el snapshot (con roster y todo) y se lo regresa. */
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

    mutex_lock(&server.mutex);

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

    mutex_unlock(&server.mutex);

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
    int lastChatSeq = 0; /* ultimo chatSeq de ESTE cliente ya procesado */

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

        mutex_lock(&server.mutex);

        /* el id de este socket ya lo fijo el handshake; packet.playerId se
         * ignora a proposito, no confiamos en lo que mande el cliente */
        Player *p = &server.game.players[myIndex];
        p->prevX = packet.x;
        p->prevY = packet.y;
        p->x = packet.x;
        p->y = packet.y;
        p->dirX = packet.dirX;
        p->dirY = packet.dirY;
        p->active = 1;

        if (packet.chatSeq != 0 && packet.chatSeq != lastChatSeq && packet.chatMsg[0] != '\0')
        {
            lastChatSeq = packet.chatSeq;

            for (int c = CHAT_HISTORY - 1; c > 0; c--)
                server.game.chatLog[c] = server.game.chatLog[c - 1];

            server.game.chatLog[0].playerId = assignedId;
            strncpy(server.game.chatLog[0].text, packet.chatMsg, CHAT_MAX_LEN - 1);
            server.game.chatLog[0].text[CHAT_MAX_LEN - 1] = '\0';

            printf("[CHAT] Jugador %d: %s\n", assignedId, server.game.chatLog[0].text);
            fflush(stdout);
        }

        buildSnapshot(&snapshot, &server.game);

        mutex_unlock(&server.mutex);

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

/* Por cada cliente nuevo, lanza su clientThread. */
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

    /* el partido siempre arranca de cero, todos inactivos hasta que cada
     * cliente se conecte (aunque el roster ya traiga IPs de antes) */
    initGame(&server.game);
    for (int i = 0; i < MAX_PLAYERS; i++)
        server.game.players[i].active = 0;

    /* si viene de una migracion el roster ya trae slotActive corregido (ver
     * computeNextHostAmongActive en network.c), se copia tal cual */
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
