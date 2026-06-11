#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"
#include "network_packet.h"

#define MAX_NETWORK_PLAYERS 4

typedef struct
{
    int id;
    float x;
    float y;
    int active;

} NetworkPlayer;

typedef struct
{
    int hostId;
    int localPlayerId;
    NetworkPlayer players[MAX_NETWORK_PLAYERS];

} NetworkState;

void initNetworkState(NetworkState *network, int localPlayerId);
void syncNetworkFromGame(NetworkState *network, GameState *game);
void printNetworkState(NetworkState *network);
void electNewHost(NetworkState *network);

int initNetworkClient(const char *serverIp);
void shutdownNetworkClient();
int sendLocalPlayerAndReceiveState(GameState *game, int localPlayerId, const char *serverIp);

#endif