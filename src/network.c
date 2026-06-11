#include "../include/network.h"
#include "../include/network_packet.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <unistd.h>

    #define SOCKET int
    #define INVALID_SOCKET -1
    #define closesocket close
#endif

void initNetworkState(NetworkState *network, int localPlayerId)
{
    network->hostId = 1;
    network->localPlayerId = localPlayerId;

    for (int i = 0; i < MAX_NETWORK_PLAYERS; i++)
    {
        network->players[i].id = i + 1;
        network->players[i].x = 0.0f;
        network->players[i].y = 0.0f;
        network->players[i].active = 0;
    }

    network->players[localPlayerId - 1].active = 1;
}

void syncNetworkFromGame(NetworkState *network, GameState *game)
{
    for (int i = 0; i < MAX_NETWORK_PLAYERS; i++)
    {
        network->players[i].id = game->players[i].id;
        network->players[i].x = game->players[i].x;
        network->players[i].y = game->players[i].y;
        network->players[i].active = game->players[i].active;
    }
}

void printNetworkState(NetworkState *network)
{
    printf("\n--- Estado de red ---\n");
    printf("Host actual: Jugador %d\n", network->hostId);
    printf("Jugador local: Jugador %d\n", network->localPlayerId);

    for (int i = 0; i < MAX_NETWORK_PLAYERS; i++)
    {
        printf(
            "Jugador %d | activo: %d | x: %.2f | y: %.2f\n",
            network->players[i].id,
            network->players[i].active,
            network->players[i].x,
            network->players[i].y);
    }

    fflush(stdout);
}

void electNewHost(NetworkState *network)
{
    int currentHostIndex = network->hostId - 1;

    if (currentHostIndex >= 0 &&
        currentHostIndex < MAX_NETWORK_PLAYERS &&
        network->players[currentHostIndex].active)
    {
        return;
    }

    for (int i = 0; i < MAX_NETWORK_PLAYERS; i++)
    {
        if (network->players[i].active)
        {
            network->hostId = network->players[i].id;

            printf("\n[NETWORK] Nuevo host: Jugador %d\n", network->hostId);
            fflush(stdout);

            return;
        }
    }
}

int initNetworkClient(const char *serverIp)
{
    (void)serverIp;

#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Error iniciando Winsock\n");
        return 0;
    }
#endif

    return 1;
}

void shutdownNetworkClient()
{
#ifdef _WIN32
    WSACleanup();
#endif
}
int sendLocalPlayerAndReceiveState(GameState *game, int localPlayerId, const char *serverIp)
{
    SOCKET sock;
    struct sockaddr_in server;
    PlayerPacket packet;
    GamePacket state;

    int index = localPlayerId - 1;

    if (index < 0 || index >= MAX_PLAYERS)
        return 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
        return 0;

    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        closesocket(sock);
        return 0;
    }

    packet.playerId = localPlayerId;
    packet.x = game->players[index].x;
    packet.y = game->players[index].y;
    packet.active = game->players[index].active;

    send(sock, (char *)&packet, sizeof(PlayerPacket), 0);

    int received = recv(sock, (char *)&state, sizeof(GamePacket), 0);

    if (received == sizeof(GamePacket))
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            game->players[i].x = state.players[i].x;
            game->players[i].y = state.players[i].y;
            game->players[i].active = state.players[i].active;
        }
    }

    closesocket(sock);
    return 1;
}