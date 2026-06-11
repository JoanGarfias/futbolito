#include <stdio.h>
#include <winsock2.h>
#include "../include/network_packet.h"

int main()
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in server, client;

    PlayerPacket packet;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5000);

    bind(serverSocket, (struct sockaddr *)&server, sizeof(server));
    listen(serverSocket, 1);

    printf("Esperando cliente con paquete...\n");

    int c = sizeof(struct sockaddr_in);

    clientSocket = accept(
        serverSocket,
        (struct sockaddr *)&client,
        &c);

    recv(clientSocket, (char *)&packet, sizeof(PlayerPacket), 0);

    printf("Paquete recibido:\n");
    printf("Jugador: %d\n", packet.playerId);
    printf("X: %.2f\n", packet.x);
    printf("Y: %.2f\n", packet.y);
    printf("Activo: %d\n", packet.active);

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}