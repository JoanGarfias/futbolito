#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H

#define MAX_PACKET_PLAYERS 4

typedef struct
{
    int playerId;
    float x;
    float y;
    float dirX;
    float dirY;
    int active;
} PlayerPacket;

typedef struct
{
    float x;
    float y;
    float vx;
    float vy;
    int size;
    int lastPlayerTouched;
} BallPacket;

typedef struct
{
    PlayerPacket players[MAX_PACKET_PLAYERS];
    BallPacket ball;
    int score[MAX_PACKET_PLAYERS];
} GamePacket;

#endif