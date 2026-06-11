#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include "../include/network_packet.h"

#define PORT 5000

void printPlayers(PlayerPacket players[])
{
    printf("\n----- ESTADO DEL SERVIDOR -----\n");

    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
    {
        printf(
            "P%d | activo: %d | x: %.2f | y: %.2f\n",
            players[i].playerId,
            players[i].active,
            players[i].x,
            players[i].y);
    }

    fflush(stdout);
}

int main()
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in server, client;

    PlayerPacket players[MAX_PACKET_PLAYERS];
    PlayerPacket packet;
    GamePacket state;

    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
    {
        players[i].playerId = i + 1;
        players[i].x = 0.0f;
        players[i].y = 0.0f;
        players[i].active = 0;
    }

    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(serverSocket, (struct sockaddr *)&server, sizeof(server));
    listen(serverSocket, 4);

    printf("Servidor de partida escuchando en puerto %d...\n", PORT);

    while (1)
    {
        int c = sizeof(struct sockaddr_in);

        clientSocket = accept(
            serverSocket,
            (struct sockaddr *)&client,
            &c);

        if (clientSocket == INVALID_SOCKET)
        {
            printf("Error aceptando cliente\n");
            continue;
        }

        int bytes = recv(clientSocket, (char *)&packet, sizeof(PlayerPacket), 0);

        if (bytes == sizeof(PlayerPacket))
        {
            int index = packet.playerId - 1;

            if (index >= 0 && index < MAX_PACKET_PLAYERS)
            {
                players[index] = packet;

                printf("\nPaquete recibido de jugador %d\n", packet.playerId);
                printPlayers(players);

                for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
                {
                    state.players[i] = players[i];
                }

                send(clientSocket, (char *)&state, sizeof(GamePacket), 0);
            }
        }
        else
        {
            printf("Paquete invalido recibido\n");
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}