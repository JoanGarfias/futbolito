/* Cliente de red, con migracion de host automatica.
 *
 * Abre una conexion TCP al host actual (con handshake de JoinRequest/
 * JoinResponse que nos da el id y el roster), lanza un hilo receptor que
 * va leyendo el estado del juego, y si el host se cae arranca un hilo de
 * reconexion en segundo plano para no trabar el bucle de SDL. El mutex de
 * NetClient protege todo lo que tocan estos hilos a la vez. */

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

/* aqui esta la definicion real del struct opaco */
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

/* Helper para asegurar que se reciben exactamente 'size' bytes. */
static int recv_all(SOCKET sock, char *buffer, int size)
{
    int total = 0;
    while (total < size)
    {
        int bytes = recv(sock, buffer + total, size - total, 0);
        if (bytes <= 0)
        {
            return bytes; /* Error o conexion cerrada */
        }
        total += bytes;
    }
    return total;
}

/* Helper para asegurar que se envian exactamente 'size' bytes. */
static int send_all(SOCKET sock, const char *buffer, int size)
{
    int total = 0;
    while (total < size)
    {
        int bytes = send(sock, buffer + total, size - total, 0);
        if (bytes <= 0)
        {
            return bytes; /* Error o conexion cerrada */
        }
        total += bytes;
    }
    return total;
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

/* Pedimos un id (0 la primera vez) y el server responde con el id que nos
 * toco y el roster completo. Si rechaza (sala llena) o falla, cierra el
 * socket y devuelve 0. */
static int doHandshake(NetClient *nc)
{
    JoinRequestPacket req;
    req.type = NET_MSG_JOIN_REQUEST;
    req.preferredId = nc->myId;

    if (send_all(nc->sock, (char *)&req, sizeof(req)) <= 0)
    {
        closesocket(nc->sock);
        nc->sock = INVALID_SOCKET;
        return 0;
    }

    JoinResponsePacket resp;
    int received = recv_all(nc->sock, (char *)&resp, sizeof(resp));

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

/* Va leyendo snapshots del servidor y los vuelca al GameState/roster
 * compartidos. No toca la posicion del jugador local, esa la maneja el
 * input del hilo principal. */
static THREAD_RET receiverThread(void *arg)
{
    NetClient *nc = (NetClient *)arg;
    GamePacket state;
    int localIndex = nc->myId - 1;

    while (nc->running)
    {
        int received = recv_all(nc->sock, (char *)&state, sizeof(GamePacket));

        if (received != (int)sizeof(GamePacket))
        {
            mutex_lock(&nc->mutex);
            nc->connected = 0;
            mutex_unlock(&nc->mutex);
            break;
        }

        mutex_lock(&nc->mutex);

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
        nc->game->ball.kickCount = state.ball.kickCount;

        for (int i = 0; i < MAX_PLAYERS; i++)
            nc->game->score[i] = state.score[i];

        nc->game->winner = state.winner;
        nc->game->goalCount = state.goalCount;

        for (int i = 0; i < CHAT_HISTORY; i++)
        {
            nc->game->chatLog[i].playerId = state.chat[i].playerId;
            strncpy(nc->game->chatLog[i].text, state.chat[i].text, CHAT_MAX_LEN - 1);
            nc->game->chatLog[i].text[CHAT_MAX_LEN - 1] = '\0';
        }

        nc->roster = state.roster;

        mutex_unlock(&nc->mutex);
    }

    return 0;
}

/* Busca el siguiente id ACTIVO en el anillo 1->2->3->4->1 a partir del host
 * caido (saltando ids vacios). Como todos parten del mismo roster, a todos
 * les da el mismo resultado sin tener que avisarse por red.
 *
 * roster no es const porque aqui mismo marcamos al host caido como inactivo:
 * si la caida fue brusca (socket muerto sin snapshot final) el roster
 * todavia puede traerlo como activo, y se auto-elegiria de nuevo si no lo
 * corregimos. Devuelve 0 si ya no queda nadie. */
static int computeNextHostAmongActive(int fallenHostId, SessionRoster *roster)
{
    if (fallenHostId >= 1 && fallenHostId <= MAX_PACKET_PLAYERS)
        roster->slotActive[fallenHostId - 1] = 0;

    for (int step = 1; step <= MAX_PACKET_PLAYERS; step++)
    {
        int id = ((fallenHostId - 1 + step) % MAX_PACKET_PLAYERS) + 1;

        if (id == fallenHostId)
            continue;

        if (!roster->slotActive[id - 1])
            continue;

        if (roster->ips[id - 1][0] == '\0')
            continue;

        return id;
    }

    return 0;
}

/* Intento inicial al arrancar el programa. Aqui si puede bloquear con
 * reintentos: todavia no hay bucle de SDL que se congele. */
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

/* Corre en su propio hilo para no bloquear SDL: cierra la conexion vieja,
 * calcula el siguiente host y reintenta hasta conectar (o hasta que
 * netDisconnect() apague nc->running). */
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
    int fallenHostId = nc->roster.currentHostId;
    SessionRoster migrationRoster = nc->roster; /* copia local: fija para todo este intento de migracion */
    mutex_unlock(&nc->mutex);

    int nextHostId = computeNextHostAmongActive(fallenHostId, &migrationRoster);

    if (nextHostId == 0)
    {
        printf("[MIGRACION] No queda ningun jugador activo. Sesion terminada.\n");
        fflush(stdout);

        mutex_lock(&nc->mutex);
        nc->roster = migrationRoster;
        nc->connected = 0;
        nc->state = NET_STATE_DISCONNECTED;
        nc->reconnectThreadRunning = 0;
        mutex_unlock(&nc->mutex);

        return 0;
    }

    migrationRoster.currentHostId = nextHostId;

    mutex_lock(&nc->mutex);
    nc->roster = migrationRoster;
    mutex_unlock(&nc->mutex);

    int iAmHost = (nc->myId == nextHostId);

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
                if (serverStartWithRoster(&migrationRoster))
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
                mutex_lock(&nc->mutex);
                int confirmedHost = (nc->roster.currentHostId == nc->myId);
                mutex_unlock(&nc->mutex);

                if (!confirmedHost)
                {
                    printf("[MIGRACION] Aviso: el servidor local no confirmo a jugador %d como host\n",
                           nc->myId);
                    fflush(stdout);
                }

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
            strncpy(targetIp, migrationRoster.ips[nextHostId - 1], sizeof(targetIp) - 1);
            targetIp[sizeof(targetIp) - 1] = '\0';

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

    /* Chat: viaja el mensaje pendiente (si hay uno) y se limpia para no
     * reenviarlo; chatSeq queda igual hasta el proximo mensaje, asi el host
     * sabe que ya lo vio aunque el texto venga vacio. */
    strncpy(packet.chatMsg, game->pendingChatMsg, CHAT_MAX_LEN - 1);
    packet.chatMsg[CHAT_MAX_LEN - 1] = '\0';
    packet.chatSeq = game->localChatSeq;
    game->pendingChatMsg[0] = '\0';
    mutex_unlock(&nc->mutex);

    if (send_all(nc->sock, (char *)&packet, sizeof(PlayerPacket)) <= 0)
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
