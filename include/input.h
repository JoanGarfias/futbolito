#ifndef INPUT_H
#define INPUT_H
#include <SDL2/SDL.h>
#include "game.h"

void handleInput(GameState *game);
void handleKeyDown(GameState *game, SDL_Keycode key);

#endif