#ifndef GAME_H
#define GAME_H

#include "player.h"
#include "ball.h"

#define MAX_PLAYERS 4

typedef struct
{
    int running;
    int score[MAX_PLAYERS];

    Player players[MAX_PLAYERS];
    Ball ball;

} GameState;

void initGame(GameState *game);
void resetBall(GameState *game);

#endif