/*
 * CLIENTE de red del Futbolito (con migracion de host).
 *
 * - Abre UNA conexion TCP persistente al host actual.
 * - Lanza un HILO RECEPTOR que lee en bucle el estado del juego y lo escribe
 *   en el GameState compartido.
 * - El hilo principal (SDL) escribe la posicion del jugador local y dibuja.
 * - Si el host se cae, elige al siguiente equipo como host y se reconecta.
 *   Si el elegido es este mismo equipo, arranca el servidor embebido.
 *
 * Como dos hilos tocan el mismo GameState, todo acceso se protege con un mutex
 * (SECCION CRITICA). Usa APIs nativas via threads.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
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
#include "../include/network.h"
#include "../include/network_packet.h"
#include "../include/server.h"

#define SERVER_PORT 5000
#define MAX_PEERS 4

/* Definicion real del tipo opaco NetClient. */
struct NetClient
{
    SOCKET sock;
    int myId;
    int running;   /* 1 mientras el hilo receptor debe seguir */
    int connected; /* 1 mientras la conexion esta viva */

    char peers[MAX_PEERS][64]; /* IPs de los equipos */
    int peerCount;
    int currentHostId; /* id del equipo que hace de host ahora */
    int hosting;       /* 1 si este proceso arranco el servidor embebido */

    GameState *game; /* estado compartido (apunta al de main) */
    Mutex mutex;     /* protege 'game' y 'connected' */
    Thread receiver;
};

/*
 * HILO RECEPTOR.
 * Lee paquetes de estado del servidor y los vuelca al GameState compartido,
 * siempre dentro de la seccion critica. No sobreescribe la posicion del
 * jugador local (esa la controla el input del hilo principal).
 */
static THREAD_RET receiverThread(void *arg)
{
    NetClient *nc = (NetClient *)arg;
    GamePacket state;
    int localIndex = nc->myId - 1;

    while (nc->running)
    {
        int received = recv(nc->sock, (char *)&state, sizeof(GamePacket), 0);

        if (received <= 0)
        {
            mutex_lock(&nc->mutex);
            nc->connected = 0;
            mutex_unlock(&nc->mutex);
            break;
        }

        if (received != (int)sizeof(GamePacket))
            continue;

        mutex_lock(&nc->mutex); /* ---- entra a seccion critica ---- */

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            nc->game->players[i].active = state.players[i].active;

            if (i == localIndex)
                continue; /* la posicion local la maneja el input */

            nc->game->players[i].x = state.players[i].x;
            nc->game->players[i].y = state.players[i].y;
        }

        nc->game->ball.x = state.ball.x;
        nc->game->ball.y = state.ball.y;
        nc->game->ball.vx = state.ball.vx;
        nc->game->ball.vy = state.ball.vy;
        nc->game->ball.size = state.ball.size;
        nc->game->ball.lastPlayerTouched = state.ball.lastPlayerTouched;

        for (int i = 0; i < MAX_PLAYERS; i++)
            nc->game->score[i] = state.score[i];

        nc->game->winner = state.winner;

        mutex_unlock(&nc->mutex); /* ---- sale de seccion critica ---- */
    }

    return 0;
}

/* Abre el socket y se conecta a la IP dada. Devuelve 1 si conecto. */
static int doConnect(NetClient *nc, const char *ip)
{
    struct sockaddr_in server;

    nc->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (nc->sock == INVALID_SOCKET)
        return 0;

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(nc->sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        socket_shutdown(nc->sock);
        closesocket(nc->sock);
        nc->sock = INVALID_SOCKET;
        return 0;
    }

    return 1;
}

/*
 * Establece conexion con el host actual. En modo punto a punto, si el host
 * soy yo arranco el servidor embebido; si el host no responde, avanzo al
 * siguiente equipo y reintento (eleccion de host).
 */
static int establishConnection(NetClient *nc)
{
    /* Modo cliente puro: una sola IP, sin migracion. */
    if (nc->peerCount <= 1)
    {
        if (doConnect(nc, nc->peers[0]))
        {
            printf("[CLIENTE] Conectado a %s como jugador %d\n",
                   nc->peers[0], nc->myId);
            fflush(stdout);
            return 1;
        }
        printf("[CLIENTE] No se pudo conectar a %s\n", nc->peers[0]);
        return 0;
    }

    /* Modo punto a punto: probamos hosts hasta encontrar uno (o ser host). */
    for (int intentos = 0; intentos < nc->peerCount; intentos++)
    {
        int hostId = nc->currentHostId;
        if (hostId < 1 || hostId > nc->peerCount)
            hostId = 1;
        nc->currentHostId = hostId;

        if (hostId == nc->myId)
        {
            /* Me toca ser host: arranco el servidor embebido (si no esta ya). */
            if (!nc->hosting)
            {
                if (serverStart())
                {
                    nc->hosting = 1;
                    printf("[HOST] Este equipo (jugador %d) ahora es el HOST\n",
                           nc->myId);
                    fflush(stdout);
                    thread_sleep_ms(300); /* dar tiempo al listen() */
                }
            }

            if (nc->hosting && doConnect(nc, "127.0.0.1"))
            {
                printf("[CLIENTE] Conectado a mi propio servidor\n");
                fflush(stdout);
                return 1;
            }
        }
        else
        {
            if (doConnect(nc, nc->peers[hostId - 1]))
            {
                printf("[CLIENTE] Conectado al host (jugador %d, %s)\n",
                       hostId, nc->peers[hostId - 1]);
                fflush(stdout);
                return 1;
            }
        }

        /* Ese host no respondio: pasamos al siguiente equipo. */
        thread_sleep_ms(500);
        nc->currentHostId++;
        if (nc->currentHostId > nc->peerCount)
            nc->currentHostId = 1;
    }

    return 0;
}

/* Reintenta la eleccion de host hasta que aparezca uno disponible.
 * Esto evita que el cliente se rinda mientras el nuevo host termina de
 * arrancar tras la caida del anterior. */
static int waitForConnection(NetClient *nc)
{
    const int retryDelayMs = 1000;

    while (nc->running)
    {
        if (establishConnection(nc))
            return 1;

        printf("[MIGRACION] Sin host disponible, reintentando en %d ms...\n",
               retryDelayMs);
        fflush(stdout);
        thread_sleep_ms(retryDelayMs);
    }

    return 0;
}

NetClient *netConnect(const char *peers[], int peerCount, int myId, GameState *game)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Error iniciando Winsock\n");
        return NULL;
    }
#endif

    if (peerCount < 1)
        peerCount = 1;
    if (peerCount > MAX_PEERS)
        peerCount = MAX_PEERS;

    NetClient *nc = (NetClient *)malloc(sizeof(NetClient));
    if (nc == NULL)
        return NULL;

    nc->myId = myId;
    nc->game = game;
    nc->running = 1;
    nc->connected = 1;
    nc->peerCount = peerCount;
    nc->currentHostId = 1; /* el host inicial es el equipo 1 */
    nc->hosting = 0;
    nc->sock = INVALID_SOCKET;

    for (int i = 0; i < peerCount; i++)
    {
        strncpy(nc->peers[i], peers[i], sizeof(nc->peers[i]) - 1);
        nc->peers[i][sizeof(nc->peers[i]) - 1] = '\0';
    }

    mutex_init(&nc->mutex);

    if (!waitForConnection(nc))
    {
        mutex_destroy(&nc->mutex);
        free(nc);
        return NULL;
    }

    if (!thread_create(&nc->receiver, receiverThread, nc))
    {
        printf("Error creando hilo receptor\n");
        closesocket(nc->sock);
        mutex_destroy(&nc->mutex);
        free(nc);
        return NULL;
    }

    return nc;
}

int netReconnect(NetClient *nc)
{
    if (nc == NULL)
        return 0;

    /* Cerramos el socket viejo primero: asi, si el hilo receptor sigue
     * bloqueado en recv(), se desbloquea y podemos esperarlo sin trabarnos. */
    if (nc->sock != INVALID_SOCKET)
    {
        socket_shutdown(nc->sock);
        closesocket(nc->sock);
        nc->sock = INVALID_SOCKET;
    }

    thread_join(nc->receiver);

    /* El host anterior cayo: el siguiente equipo toma el relevo. */
    nc->currentHostId++;
    if (nc->currentHostId > nc->peerCount)
        nc->currentHostId = 1;

    printf("[MIGRACION] Buscando nuevo host a partir del jugador %d...\n",
           nc->currentHostId);
    fflush(stdout);

    if (!waitForConnection(nc))
    {
        nc->connected = 0;
        return 0;
    }

    nc->connected = 1;
    printf("[MIGRACION] Nuevo host: jugador %d\n", nc->currentHostId);
    fflush(stdout);

    if (!thread_create(&nc->receiver, receiverThread, nc))
    {
        nc->connected = 0;
        return 0;
    }

    return 1;
}

void netSendLocalPlayer(NetClient *nc, GameState *game)
{
    if (nc == NULL || !nc->connected)
        return;

    int index = nc->myId - 1;
    if (index < 0 || index >= MAX_PLAYERS)
        return;

    PlayerPacket packet;

    mutex_lock(&nc->mutex);
    packet.playerId = nc->myId;
    packet.x = game->players[index].x;
    packet.y = game->players[index].y;
    packet.dirX = game->players[index].dirX;
    packet.dirY = game->players[index].dirY;
    packet.active = 1;
    mutex_unlock(&nc->mutex);

    if (send(nc->sock, (char *)&packet, sizeof(PlayerPacket), 0) <= 0)
    {
        mutex_lock(&nc->mutex);
        nc->connected = 0;
        mutex_unlock(&nc->mutex);
    }
}

void netLockState(NetClient *nc)
{
    if (nc != NULL)
        mutex_lock(&nc->mutex);
}

void netUnlockState(NetClient *nc)
{
    if (nc != NULL)
        mutex_unlock(&nc->mutex);
}

int netIsConnected(NetClient *nc)
{
    if (nc == NULL)
        return 0;

    int c;
    mutex_lock(&nc->mutex);
    c = nc->connected;
    mutex_unlock(&nc->mutex);
    return c;
}

int netCurrentHost(NetClient *nc)
{
    return (nc != NULL) ? nc->currentHostId : 0;
}

void netDisconnect(NetClient *nc)
{
    if (nc == NULL)
        return;

    nc->running = 0;

    if (nc->sock != INVALID_SOCKET)
    {
        socket_shutdown(nc->sock);
        closesocket(nc->sock); /* desbloquea el recv() del hilo receptor */
    }

    thread_join(nc->receiver);

    if (nc->hosting)
        serverStop();

    mutex_destroy(&nc->mutex);
    free(nc);

#ifdef _WIN32
    WSACleanup();
#endif
}
