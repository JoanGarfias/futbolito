#include "../include/player.h"

void initPlayer(Player *player, int id, float x, float y)
{
    player->id = id;
    player->x = x;
    player->y = y;

    player->prevX = x;
    player->prevY = y;

    player->dirX = 1.0f;
    player->dirY = 0.0f;

    player->speed = 5.0f;
    player->width = 30;
    player->height = 30;
    player->active = 1;
    player->joinOrder = 0;
}