#include "../include/physics.h"
#include <stdio.h>

#define FIELD_LEFT 50
#define FIELD_TOP 50
#define FIELD_RIGHT 950
#define FIELD_BOTTOM 550

#define GOAL_SIZE 100
#define KICK_POWER 8.0f

int checkCollision(float x1, float y1, int w1, int h1,
                   float x2, float y2, int w2, int h2)
{
    return (
        x1 < x2 + w2 &&
        x1 + w1 > x2 &&
        y1 < y2 + h2 &&
        y1 + h1 > y2);
}

void registerGoal(GameState *game, int goalOwner)
{
    int scorer = game->ball.lastPlayerTouched;

    if (scorer == -1)
    {
        resetBall(game);
        return;
    }

    if (scorer == goalOwner)
    {
        game->score[scorer]--;
        printf("[GOL] Autogol del Jugador %d", scorer + 1);
    }
    else
    {
        game->score[scorer]++;
        printf("[GOL] Anota el Jugador %d", scorer + 1);
    }

    printf(" | Marcador: P1=%d P2=%d P3=%d P4=%d\n",
           game->score[0], game->score[1], game->score[2], game->score[3]);
    fflush(stdout);

    /* Si el goleador llego a los goles necesarios, gana la partida. */
    if (game->score[scorer] >= GOALS_TO_WIN)
    {
        game->winner = scorer;
        printf("[JUEGO] El Jugador %d GANA la partida!\n", scorer + 1);
        fflush(stdout);
    }

    resetBall(game);
}

void updatePhysics(GameState *game)
{
    Ball *ball = &game->ball;

    /* Si ya hay ganador, congelamos el juego (no se mueve la pelota). */
    if (game->winner != -1)
        return;

    // Colisión jugador-balón
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *player = &game->players[i];

        if (!player->active)
            continue;

        if (checkCollision(
                player->x, player->y, player->width, player->height,
                ball->x, ball->y, ball->size, ball->size))
        {
            ball->vx = player->dirX * KICK_POWER;
            ball->vy = player->dirY * KICK_POWER;
            ball->lastPlayerTouched = i;

            if (ball->vx == 0.0f && ball->vy == 0.0f)
            {
                ball->vx = 4.0f;
            }
        }
    }

    // Colisión jugador-jugador
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        Player *a = &game->players[i];

        if (!a->active)
            continue;

        for (int j = i + 1; j < MAX_PLAYERS; j++)
        {
            Player *b = &game->players[j];

            if (!b->active)
                continue;

            if (checkCollision(
                    a->x, a->y, a->width, a->height,
                    b->x, b->y, b->width, b->height))
            {
                a->x = a->prevX;
                a->y = a->prevY;

                b->x = b->prevX;
                b->y = b->prevY;
            }
        }
    }

    ball->x += ball->vx;
    ball->y += ball->vy;

    int goalCenterX = 500;
    int goalCenterY = 300;

    // Portería izquierda - Jugador 1
    if (
        game->players[0].active &&
        ball->x < FIELD_LEFT &&
        ball->y + ball->size > goalCenterY - GOAL_SIZE / 2 &&
        ball->y < goalCenterY + GOAL_SIZE / 2)
    {
        registerGoal(game, 0);
        return;
    }

    // Portería derecha - Jugador 2
    if (
        game->players[1].active &&
        ball->x + ball->size > FIELD_RIGHT &&
        ball->y + ball->size > goalCenterY - GOAL_SIZE / 2 &&
        ball->y < goalCenterY + GOAL_SIZE / 2)
    {
        registerGoal(game, 1);
        return;
    }

    // Portería arriba - Jugador 3
    if (
        game->players[2].active &&
        ball->y < FIELD_TOP &&
        ball->x + ball->size > goalCenterX - GOAL_SIZE / 2 &&
        ball->x < goalCenterX + GOAL_SIZE / 2)
    {
        registerGoal(game, 2);
        return;
    }

    // Portería abajo - Jugador 4
    if (
        game->players[3].active &&
        ball->y + ball->size > FIELD_BOTTOM &&
        ball->x + ball->size > goalCenterX - GOAL_SIZE / 2 &&
        ball->x < goalCenterX + GOAL_SIZE / 2)
    {
        registerGoal(game, 3);
        return;
    }

    // Rebotes
    if (ball->x < FIELD_LEFT)
    {
        ball->x = FIELD_LEFT;
        ball->vx *= -0.8f;
    }

    if (ball->x + ball->size > FIELD_RIGHT)
    {
        ball->x = FIELD_RIGHT - ball->size;
        ball->vx *= -0.8f;
    }

    if (ball->y < FIELD_TOP)
    {
        ball->y = FIELD_TOP;
        ball->vy *= -0.8f;
    }

    if (ball->y + ball->size > FIELD_BOTTOM)
    {
        ball->y = FIELD_BOTTOM - ball->size;
        ball->vy *= -0.8f;
    }

    // Fricción
    ball->vx *= 0.985f;
    ball->vy *= 0.985f;

    if (ball->vx > -0.05f && ball->vx < 0.05f)
        ball->vx = 0.0f;

    if (ball->vy > -0.05f && ball->vy < 0.05f)
        ball->vy = 0.0f;
}