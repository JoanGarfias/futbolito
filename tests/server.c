#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;

    struct sockaddr_in server, client;

    char buffer[1024];

    WSAStartup(MAKEWORD(2,2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5000);

    bind(serverSocket, (struct sockaddr*)&server, sizeof(server));

    listen(serverSocket, 1);

    printf("Esperando cliente...\n");

    int c = sizeof(struct sockaddr_in);

    clientSocket = accept(
        serverSocket,
        (struct sockaddr*)&client,
        &c
    );

    recv(clientSocket, buffer, sizeof(buffer), 0);

    printf("Mensaje recibido: %s\n", buffer);

    int id;
float x, y;

sscanf(buffer, "%d %f %f", &id, &x, &y);

printf("Jugador: %d\n", id);
printf("X: %.2f\n", x);
printf("Y: %.2f\n", y);

    closesocket(clientSocket);
    closesocket(serverSocket);

    WSACleanup();

    return 0;
}