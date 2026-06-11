#include <stdio.h>
#include <winsock2.h>
#include "../include/network_packet.h"

#define PORT 5000

int main()
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    PlayerPacket packet;
    packet.playerId = 2;
    packet.x = 100.0f;
    packet.y = 200.0f;
    packet.active = 1;

    for (int i = 0; i < 5; i++)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);

        server.sin_family = AF_INET;
        server.sin_port = htons(PORT);
        server.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            printf("No se pudo conectar al servidor\n");
            closesocket(sock);
            continue;
        }

        packet.x += 20.0f;
        packet.y += 10.0f;

        send(sock, (char *)&packet, sizeof(PlayerPacket), 0);

        printf("Enviado P%d | x: %.2f | y: %.2f\n",
               packet.playerId,
               packet.x,
               packet.y);

        GamePacket state;

        int received = recv(sock, (char *)&state, sizeof(GamePacket), 0);

        if (received == sizeof(GamePacket))
        {
            printf("\n--- Estado recibido del servidor ---\n");

            for (int j = 0; j < MAX_PACKET_PLAYERS; j++)
            {
                printf(
                    "P%d | activo: %d | x: %.2f | y: %.2f\n",
                    state.players[j].playerId,
                    state.players[j].active,
                    state.players[j].x,
                    state.players[j].y);
            }
        }
        else
        {
            printf("No se recibio estado completo del servidor\n");
        }

        closesocket(sock);

        Sleep(500);
    }

    WSACleanup();

    return 0;
}