#ifndef BALL_H
#define BALL_H

typedef struct
{
    float x;
    float y;
    float vx;
    float vy;

    int size;

    int lastPlayerTouched;
    int kickCount; /* sube cada vez que un jugador la golpea (para el sonido) */

} Ball;

void initBall(Ball *ball, float x, float y);

#endif