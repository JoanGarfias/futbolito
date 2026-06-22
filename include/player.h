#ifndef PLAYER_H
#define PLAYER_H

typedef struct
{
    float x;
    float y;
    float speed;

    float dirX;
    float dirY;

    float prevX;
    float prevY;

    int width;
    int height;
    int active;
    int id;
    int joinOrder;

} Player;

void initPlayer(Player *player, int id, float x, float y);

#endif