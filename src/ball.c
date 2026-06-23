#include "../include/ball.h"

void initBall(Ball *ball, float x, float y)
{
    ball->x = x;
    ball->y = y;
    ball->vx = 0.0f;
    ball->vy = 0.0f;
    ball->size = 20;
    ball->lastPlayerTouched = -1;
    ball->kickCount = 0;
}
