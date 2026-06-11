#include <stdio.h>
#include <winsock2.h>
#include "../include/network_packet.h"

int main()
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    PlayerPacket packet;
    packet.playerId = 2;
    packet.x = 300.0f;
    packet.y = 400.0f;
    packet.active = 1;

    send(sock, (char *)&packet, sizeof(PlayerPacket), 0);

    closesocket(sock);
    WSACleanup();

    return 0;
}