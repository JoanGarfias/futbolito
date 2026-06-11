#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H

#define MAX_PACKET_PLAYERS 4

typedef struct
{
    int playerId;
    float x;
    float y;
    int active;

} PlayerPacket;

typedef struct
{
    PlayerPacket players[MAX_PACKET_PLAYERS];

} GamePacket;

#endif