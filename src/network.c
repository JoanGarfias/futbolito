/*
 * CLIENTE de red del Futbolito (con migracion de host automatica).
 *
 * - Abre UNA conexion TCP persistente al host actual, con un handshake
 *   (JoinRequest/JoinResponse) que asigna el id de jugador y entrega el
 *   roster de la sesion (ver network_packet.h).
 * - Lanza un HILO RECEPTOR que lee en bucle el estado del juego (con el
 *   roster embebido) y lo escribe en el GameState/roster compartidos.
 * - El hilo principal (SDL) escribe la posicion del jugador local y dibuja.
 * - Si el host se cae, un HILO DE RECONEXION en segundo plano elige al
 *   siguiente equipo segun el roster y se reconecta, sin bloquear el bucle
 *   SDL. Si el elegido es este mismo equipo, arranca el servidor embebido.
 *
 * Como varios hilos tocan el mismo GameState/NetClient, todo acceso se
 * protege con un mutex (SECCION CRITICA). Usa APIs nativas via threads.h.
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

/* Definicion real del tipo opaco NetClient. */
struct NetClient
{
    SOCKET sock;
    int myId; /* 0 hasta que el servidor lo asigna en el handshake */
    int running;   /* 1 mientras los hilos de fondo deben seguir */
    int connected; /* 1 mientras la conexion actual esta viva */

    char hostIp[64];      /* IP a la que estamos (o queremos estar) conectados */
    int isSessionHost;    /* 1 si este proceso creo la sesion (--host) */
    SessionRoster roster; /* copia local, se actualiza con cada snapshot */

    int hosting; /* 1 si este proceso arranco el servidor embebido */

    NetState state;
    int receiverThreadActive;
    int reconnectThreadRunning;

    GameState *game; /* estado compartido (apunta al de main) */
    Mutex mutex;      /* protege 'game', 'roster', 'connected', 'state', ... */
    Thread receiver;
    Thread reconnectThread;
};

static THREAD_RET receiverThread(void *arg);
static THREAD_RET reconnectWorker(void *arg);

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
 * Handshake de aplicacion sobre la conexion TCP ya abierta: pedimos un id
 * (preferredId = nc->myId, 0 la primera vez) y el servidor nos responde con
 * el id asignado y el roster completo de la sesion. Si el servidor rechaza
 * (sala llena) o falla el intercambio, cierra el socket y devuelve 0.
 */
static int doHandshake(NetClient *nc)
{
    JoinRequestPacket req;
    req.type = NET_MSG_JOIN_REQUEST;
    req.preferredId = nc->myId;

    if (send(nc->sock, (char *)&req, sizeof(req), 0) <= 0)
    {
        closesocket(nc->sock);
        nc->sock = INVALID_SOCKET;
        return 0;
    }

    JoinResponsePacket resp;
    int received = recv(nc->sock, (char *)&resp, sizeof(resp), 0);

    if (received != (int)sizeof(resp) || resp.type != NET_MSG_JOIN_RESPONSE ||
        resp.assignedId == 0)
    {
        if (received == (int)sizeof(resp) && resp.assignedId == 0)
            printf("[CLIENTE] El servidor rechazo la conexion (sala llena)\n");
        else
            printf("[CLIENTE] Handshake fallido con el host\n");
        fflush(stdout);

        socket_shutdown(nc->sock);
        closesocket(nc->sock);
        nc->sock = INVALID_SOCKET;
        return 0;
    }

    mutex_lock(&nc->mutex);
    nc->myId = resp.assignedId;
    nc->roster = resp.roster;
    mutex_unlock(&nc->mutex);

    printf("[CLIENTE] Conectado. Soy el jugador %d. Host actual: jugador %d\n",
           resp.assignedId, resp.roster.currentHostId);
    fflush(stdout);

    return 1;
}

/*
 * HILO RECEPTOR.
 * Lee paquetes de estado del servidor (con el roster embebido) y los vuelca
 * al GameState/roster compartidos, siempre dentro de la seccion critica. No
 * sobreescribe la posicion del jugador local (esa la controla el input del
 * hilo principal).
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

        nc->roster = state.roster; /* roster sincronizado en cada snapshot */

        mutex_unlock(&nc->mutex); /* ---- sale de seccion critica ---- */
    }

    return 0;
}

/*
 * Calcula, de forma deterministica, el id del siguiente host a partir del id
 * del host caido: el siguiente id REGISTRADO en el roster (wrap de 4 a 1).
 * Todos los clientes parten del mismo roster y del mismo previousHostId, asi
 * que todos llegan al mismo resultado sin coordinarse por red.
 */
static int computeNextHostId(int previousHostId, const SessionRoster *roster)
{
    if (previousHostId < 1 || previousHostId > MAX_PACKET_PLAYERS)
        previousHostId = MAX_PACKET_PLAYERS;

    for (int step = 1; step <= MAX_PACKET_PLAYERS; step++)
    {
        int candidate = ((previousHostId - 1 + step) % MAX_PACKET_PLAYERS) + 1;
        if (roster->slotUsed[candidate - 1])
            return candidate;
    }

    return 1; /* nadie registrado: no deberia pasar, pero hay que devolver algo */
}

/*
 * Intento de conexion inicial (al arrancar el programa). Puede bloquear con
 * reintentos acotados: en este punto todavia no hay bucle SDL que congelar.
 */
static int initialConnect(NetClient *nc)
{
    const int maxAttempts = 20;
    const int retryDelayMs = 1000;

    for (int attempt = 1; attempt <= maxAttempts && nc->running; attempt++)
    {
        if (nc->isSessionHost)
        {
            if (!nc->hosting)
            {
                if (serverStartWithRoster(NULL))
                {
                    nc->hosting = 1;
                    printf("[HOST] Sesion creada. Este equipo es el HOST.\n");
                    fflush(stdout);
                    thread_sleep_ms(500); /* dar tiempo al bind()/listen() local */
                }
                else
                {
                    printf("[HOST] No se pudo arrancar el servidor (puerto %d ocupado?)\n",
                           SERVER_PORT);
                    fflush(stdout);
                    return 0; /* fallo irrecuperable: no insistir */
                }
            }

            if (doConnect(nc, "127.0.0.1") && doHandshake(nc))
            {
                strncpy(nc->hostIp, "127.0.0.1", sizeof(nc->hostIp) - 1);
                return 1;
            }
        }
        else
        {
            printf("[CLIENTE] Conectando a %s... (intento %d/%d)\n",
                   nc->hostIp, attempt, maxAttempts);
            fflush(stdout);

            if (doConnect(nc, nc->hostIp) && doHandshake(nc))
                return 1;
        }

        thread_sleep_ms(retryDelayMs);
    }

    return 0;
}

NetClient *netConnect(const char *hostIp, int isSessionHost, GameState *game)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Error iniciando Winsock\n");
        return NULL;
    }
#endif

    NetClient *nc = (NetClient *)malloc(sizeof(NetClient));
    if (nc == NULL)
        return NULL;

    nc->myId = 0;
    nc->game = game;
    nc->running = 1;
    nc->connected = 0;
    nc->isSessionHost = isSessionHost;
    nc->hosting = 0;
    nc->sock = INVALID_SOCKET;
    nc->state = NET_STATE_RECONNECTING;
    nc->receiverThreadActive = 0;
    nc->reconnectThreadRunning = 0;
    memset(&nc->roster, 0, sizeof(nc->roster));

    strncpy(nc->hostIp, hostIp, sizeof(nc->hostIp) - 1);
    nc->hostIp[sizeof(nc->hostIp) - 1] = '\0';

    mutex_init(&nc->mutex);

    if (!initialConnect(nc))
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

    nc->receiverThreadActive = 1;
    nc->connected = 1;
    nc->state = NET_STATE_CONNECTED;

    return nc;
}

int netGetMyId(NetClient *nc)
{
    return (nc != NULL) ? nc->myId : 0;
}

NetState netGetState(NetClient *nc)
{
    if (nc == NULL)
        return NET_STATE_DISCONNECTED;

    NetState s;
    mutex_lock(&nc->mutex);
    s = nc->state;
    mutex_unlock(&nc->mutex);
    return s;
}

void netBeginReconnect(NetClient *nc)
{
    if (nc == NULL)
        return;

    mutex_lock(&nc->mutex);
    int alreadyRunning = nc->reconnectThreadRunning;
    if (!alreadyRunning)
    {
        nc->state = NET_STATE_RECONNECTING;
        nc->reconnectThreadRunning = 1;
    }
    mutex_unlock(&nc->mutex);

    if (alreadyRunning)
        return;

    if (!thread_create(&nc->reconnectThread, reconnectWorker, nc))
    {
        mutex_lock(&nc->mutex);
        nc->state = NET_STATE_DISCONNECTED;
        nc->reconnectThreadRunning = 0;
        mutex_unlock(&nc->mutex);
    }
}

int netPollReconnect(NetClient *nc)
{
    return netIsConnected(nc);
}

/*
 * Trabajo de migracion, corre en un hilo aparte para no bloquear el bucle
 * SDL. Cierra la conexion vieja, calcula el siguiente host (deterministico a
 * partir del roster) y reintenta hasta conectar (o hasta que nc->running se
 * apague por netDisconnect()).
 */
static THREAD_RET reconnectWorker(void *arg)
{
    NetClient *nc = (NetClient *)arg;
    const int retryDelayMs = 1500;

    if (nc->sock != INVALID_SOCKET)
    {
        socket_shutdown(nc->sock);
        closesocket(nc->sock);
        nc->sock = INVALID_SOCKET;
    }

    if (nc->receiverThreadActive)
    {
        thread_join(nc->receiver);
        nc->receiverThreadActive = 0;
    }

    mutex_lock(&nc->mutex);
    int nextHostId = computeNextHostId(nc->roster.currentHostId, &nc->roster);
    nc->roster.currentHostId = nextHostId;
    int iAmHost = (nc->myId == nextHostId);
    mutex_unlock(&nc->mutex);

    if (iAmHost)
        printf("[MIGRACION] Soy el jugador %d: paso a ser el nuevo HOST.\n", nc->myId);
    else
        printf("[MIGRACION] Esperando al nuevo host: jugador %d.\n", nextHostId);
    fflush(stdout);

    int connected = 0;

    while (nc->running && !connected)
    {
        if (iAmHost)
        {
            if (!nc->hosting)
            {
                SessionRoster snapshot;
                mutex_lock(&nc->mutex);
                snapshot = nc->roster;
                mutex_unlock(&nc->mutex);

                if (serverStartWithRoster(&snapshot))
                {
                    nc->hosting = 1;
                    printf("[HOST] Este equipo (jugador %d) ahora es el HOST\n", nc->myId);
                    fflush(stdout);
                    thread_sleep_ms(500); /* dar tiempo al bind()/listen() local */
                }
                else
                {
                    printf("[MIGRACION] Puerto %d ocupado, reintentando en %d ms...\n",
                           SERVER_PORT, retryDelayMs);
                    fflush(stdout);
                }
            }

            if (nc->hosting && doConnect(nc, "127.0.0.1") && doHandshake(nc))
            {
                strncpy(nc->hostIp, "127.0.0.1", sizeof(nc->hostIp) - 1);
                connected = 1;
                break;
            }
        }
        else
        {
            if (nc->hosting)
            {
                /* Servidor arrancado por error en una carrera anterior: ya
                 * no nos toca, lo apagamos. */
                serverStop();
                nc->hosting = 0;
            }

            char targetIp[64];
            mutex_lock(&nc->mutex);
            strncpy(targetIp, nc->roster.ips[nextHostId - 1], sizeof(targetIp) - 1);
            targetIp[sizeof(targetIp) - 1] = '\0';
            mutex_unlock(&nc->mutex);

            if (targetIp[0] != '\0' && doConnect(nc, targetIp) && doHandshake(nc))
            {
                strncpy(nc->hostIp, targetIp, sizeof(nc->hostIp) - 1);
                connected = 1;
                break;
            }

            printf("[MIGRACION] Esperando al host elegido (jugador %d), reintentando en %d ms...\n",
                   nextHostId, retryDelayMs);
            fflush(stdout);
        }

        thread_sleep_ms(retryDelayMs);
    }

    if (connected && thread_create(&nc->receiver, receiverThread, nc))
    {
        nc->receiverThreadActive = 1;

        mutex_lock(&nc->mutex);
        nc->connected = 1;
        nc->state = NET_STATE_CONNECTED;
        nc->reconnectThreadRunning = 0;
        mutex_unlock(&nc->mutex);

        printf("[MIGRACION] Reconectado. Host actual: jugador %d\n", nextHostId);
        fflush(stdout);
    }
    else
    {
        mutex_lock(&nc->mutex);
        nc->connected = 0;
        nc->state = NET_STATE_DISCONNECTED;
        nc->reconnectThreadRunning = 0;
        mutex_unlock(&nc->mutex);

        if (nc->running)
        {
            printf("[MIGRACION] No hay ningun host disponible.\n");
            fflush(stdout);
        }
    }

    return 0;
}

void netSendLocalPlayer(NetClient *nc, GameState *game)
{
    if (nc == NULL)
        return;

    mutex_lock(&nc->mutex);
    int canSend = (nc->state == NET_STATE_CONNECTED);
    int myId = nc->myId;
    mutex_unlock(&nc->mutex);

    if (!canSend)
        return;

    int index = myId - 1;
    if (index < 0 || index >= MAX_PLAYERS)
        return;

    PlayerPacket packet;

    mutex_lock(&nc->mutex);
    packet.playerId = myId;
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
    if (nc == NULL)
        return 0;

    int h;
    mutex_lock(&nc->mutex);
    h = nc->roster.currentHostId;
    mutex_unlock(&nc->mutex);
    return h;
}

void netDisconnect(NetClient *nc)
{
    if (nc == NULL)
        return;

    nc->running = 0;

    mutex_lock(&nc->mutex);
    int reconnecting = nc->reconnectThreadRunning;
    mutex_unlock(&nc->mutex);

    if (reconnecting)
        thread_join(nc->reconnectThread);

    if (nc->sock != INVALID_SOCKET)
    {
        socket_shutdown(nc->sock);
        closesocket(nc->sock); /* desbloquea el recv() del hilo receptor */
    }

    if (nc->receiverThreadActive)
    {
        thread_join(nc->receiver);
        nc->receiverThreadActive = 0;
    }

    if (nc->hosting)
        serverStop();

    mutex_destroy(&nc->mutex);
    free(nc);

#ifdef _WIN32
    WSACleanup();
#endif
}
