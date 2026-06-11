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

} Ball;

void initBall(Ball *ball, float x, float y);

#endif