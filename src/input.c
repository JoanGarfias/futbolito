#include <SDL2/SDL.h>
#include "../include/input.h"

#define FIELD_LEFT 50
#define FIELD_TOP 50
#define FIELD_RIGHT 950
#define FIELD_BOTTOM 550

void handleInput(GameState *game)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    Player *player = &game->players[0];

    if (!player->active)
        return;

    player->prevX = player->x;
    player->prevY = player->y;

    float moveX = 0.0f;
    float moveY = 0.0f;

    if (keys[SDL_SCANCODE_W])
        moveY -= 1.0f;

    if (keys[SDL_SCANCODE_S])
        moveY += 1.0f;

    if (keys[SDL_SCANCODE_A])
        moveX -= 1.0f;

    if (keys[SDL_SCANCODE_D])
        moveX += 1.0f;

    if (moveX != 0.0f || moveY != 0.0f)
    {
        player->dirX = moveX;
        player->dirY = moveY;

        player->x += moveX * player->speed;
        player->y += moveY * player->speed;
    }

    if (player->x < FIELD_LEFT)
        player->x = FIELD_LEFT;

    if (player->y < FIELD_TOP)
        player->y = FIELD_TOP;

    if (player->x + player->width > FIELD_RIGHT)
        player->x = FIELD_RIGHT - player->width;

    if (player->y + player->height > FIELD_BOTTOM)
        player->y = FIELD_BOTTOM - player->height;
}

void handleKeyDown(GameState *game, SDL_Keycode key)
{
    if (key == SDLK_1)
        game->players[0].active = !game->players[0].active;

    if (key == SDLK_2)
        game->players[1].active = !game->players[1].active;

    if (key == SDLK_3)
        game->players[2].active = !game->players[2].active;

    if (key == SDLK_4)
        game->players[3].active = !game->players[3].active;
}