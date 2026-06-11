#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsa;
    SOCKET sock;

    struct sockaddr_in server;

    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("192.168.8.195");

    connect(
        sock,
        (struct sockaddr*)&server,
        sizeof(server)
    );

    char mensaje[] = "1 250 320";

send(sock, mensaje, sizeof(mensaje), 0);

    closesocket(sock);

    WSACleanup();

    return 0;
}