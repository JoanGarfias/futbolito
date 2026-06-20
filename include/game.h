#ifndef GAME_H
#define GAME_H

#include "player.h"
#include "ball.h"

#define MAX_PLAYERS 4

/* Goles necesarios para ganar la partida. */
#define GOALS_TO_WIN 5

typedef struct
{
    int running;
    int score[MAX_PLAYERS];
    int winner; /* -1 si nadie ha ganado; si no, indice del jugador ganador */

    Player players[MAX_PLAYERS];
    Ball ball;

} GameState;

void initGame(GameState *game);
void resetBall(GameState *game);

#endif