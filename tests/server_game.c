#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include "../include/network_packet.h"

#define PORT 5000

#define FIELD_LEFT 50
#define FIELD_TOP 50
#define FIELD_RIGHT 950
#define FIELD_BOTTOM 550
#define GOAL_SIZE 100

#define PLAYER_SIZE 30
#define BALL_SIZE 20

void resetBall(GamePacket *state)
{
    state->ball.x = 490;
    state->ball.y = 290;
    state->ball.vx = 0.0f;
    state->ball.vy = 0.0f;
    state->ball.size = BALL_SIZE;
    state->ball.lastPlayerTouched = -1;
}

int checkCollision(float x1, float y1, int w1, int h1,
                   float x2, float y2, int w2, int h2)
{
    return (
        x1 < x2 + w2 &&
        x1 + w1 > x2 &&
        y1 < y2 + h2 &&
        y1 + h1 > y2);
}

void registerGoal(GamePacket *state, int goalOwner)
{
    int scorer = state->ball.lastPlayerTouched;

    if (scorer >= 0 && scorer < MAX_PACKET_PLAYERS)
    {
        if (scorer == goalOwner)
            state->score[scorer]--;
        else
            state->score[scorer]++;
    }

    resetBall(state);
}

void updateServerPhysics(GamePacket *state)
{
    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
    {
        if (!state->players[i].active)
            continue;

        if (checkCollision(
                state->players[i].x,
                state->players[i].y,
                PLAYER_SIZE,
                PLAYER_SIZE,
                state->ball.x,
                state->ball.y,
                state->ball.size,
                state->ball.size))
        {
            float dx = state->ball.x - state->players[i].x;
            float dy = state->ball.y - state->players[i].y;

            if (dx > 0)
                state->ball.vx = 8.0f;
            else
                state->ball.vx = -8.0f;

            if (dy > 0)
                state->ball.vy = 4.0f;
            else
                state->ball.vy = -4.0f;

            state->ball.lastPlayerTouched = i;
        }
    }

    state->ball.x += state->ball.vx;
    state->ball.y += state->ball.vy;

    int goalCenterX = 500;
    int goalCenterY = 300;

    if (state->players[0].active &&
        state->ball.x < FIELD_LEFT &&
        state->ball.y + state->ball.size > goalCenterY - GOAL_SIZE / 2 &&
        state->ball.y < goalCenterY + GOAL_SIZE / 2)
    {
        registerGoal(state, 0);
        return;
    }

    if (state->players[1].active &&
        state->ball.x + state->ball.size > FIELD_RIGHT &&
        state->ball.y + state->ball.size > goalCenterY - GOAL_SIZE / 2 &&
        state->ball.y < goalCenterY + GOAL_SIZE / 2)
    {
        registerGoal(state, 1);
        return;
    }

    if (state->players[2].active &&
        state->ball.y < FIELD_TOP &&
        state->ball.x + state->ball.size > goalCenterX - GOAL_SIZE / 2 &&
        state->ball.x < goalCenterX + GOAL_SIZE / 2)
    {
        registerGoal(state, 2);
        return;
    }

    if (state->players[3].active &&
        state->ball.y + state->ball.size > FIELD_BOTTOM &&
        state->ball.x + state->ball.size > goalCenterX - GOAL_SIZE / 2 &&
        state->ball.x < goalCenterX + GOAL_SIZE / 2)
    {
        registerGoal(state, 3);
        return;
    }

    if (state->ball.x < FIELD_LEFT)
    {
        state->ball.x = FIELD_LEFT;
        state->ball.vx *= -0.8f;
    }

    if (state->ball.x + state->ball.size > FIELD_RIGHT)
    {
        state->ball.x = FIELD_RIGHT - state->ball.size;
        state->ball.vx *= -0.8f;
    }

    if (state->ball.y < FIELD_TOP)
    {
        state->ball.y = FIELD_TOP;
        state->ball.vy *= -0.8f;
    }

    if (state->ball.y + state->ball.size > FIELD_BOTTOM)
    {
        state->ball.y = FIELD_BOTTOM - state->ball.size;
        state->ball.vy *= -0.8f;
    }

    state->ball.vx *= 0.985f;
    state->ball.vy *= 0.985f;

    if (state->ball.vx > -0.05f && state->ball.vx < 0.05f)
        state->ball.vx = 0.0f;

    if (state->ball.vy > -0.05f && state->ball.vy < 0.05f)
        state->ball.vy = 0.0f;
}

int main()
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in server, client;

    PlayerPacket packet;
    GamePacket state;

    for (int i = 0; i < MAX_PACKET_PLAYERS; i++)
    {
        state.players[i].playerId = i + 1;
        state.players[i].x = 0.0f;
        state.players[i].y = 0.0f;
        state.players[i].active = 0;
        state.score[i] = 0;
    }

    resetBall(&state);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(serverSocket, (struct sockaddr *)&server, sizeof(server));
    listen(serverSocket, 4);

    printf("Servidor escuchando en puerto %d...\n", PORT);

    while (1)
    {
        int c = sizeof(struct sockaddr_in);

        clientSocket = accept(serverSocket, (struct sockaddr *)&client, &c);

        if (clientSocket == INVALID_SOCKET)
            continue;

        int bytes = recv(clientSocket, (char *)&packet, sizeof(PlayerPacket), 0);

        if (bytes == sizeof(PlayerPacket))
        {
            int index = packet.playerId - 1;

            if (index >= 0 && index < MAX_PACKET_PLAYERS)
            {
                state.players[index] = packet;

                updateServerPhysics(&state);

                send(clientSocket, (char *)&state, sizeof(GamePacket), 0);

                printf("P%d | x: %.2f | y: %.2f | Ball: %.2f %.2f\n",
                       packet.playerId,
                       packet.x,
                       packet.y,
                       state.ball.x,
                       state.ball.y);

                fflush(stdout);
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}