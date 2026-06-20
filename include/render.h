#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"

/* Carga los sprites (BMP) de jugadores y pelota. Si un archivo no existe,
 * ese elemento se dibujara con su color solido de respaldo. Llamar una vez
 * despues de crear el renderer. */
void loadSprites(SDL_Renderer *renderer);

/* Libera las texturas de los sprites. Llamar antes de cerrar SDL. */
void freeSprites(void);

void renderGame(SDL_Renderer *renderer, TTF_Font *font, GameState *game);

#endif
