#include "../include/game.h"

void resetBall(GameState *game)
{
    game->ball.x = 490;
    game->ball.y = 290;

    game->ball.vx = 0.0f;
    game->ball.vy = 0.0f;

    game->ball.lastPlayerTouched = -1;
}

void initGame(GameState *game)
{
    game->running = 1;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        game->score[i] = 0;
    }

    initPlayer(&game->players[0], 1, 120, 285); // izquierda
    initPlayer(&game->players[1], 2, 850, 285); // derecha
    initPlayer(&game->players[2], 3, 485, 100); // arriba
    initPlayer(&game->players[3], 4, 485, 470); // abajo

    initBall(&game->ball, 490, 290);
}