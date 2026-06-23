#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"

/* Si falta algun BMP, ese elemento se dibuja con su color solido de
 * respaldo. Llamar una vez despues de crear el renderer. */
void loadSprites(SDL_Renderer *renderer);
void freeSprites(void);

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);

/* No llama SDL_RenderPresent: el caller decide cuando presentar, para poder
 * dibujar overlays (reconectando, pausa...) encima primero. */
void renderGame(SDL_Renderer *renderer, TTF_Font *font, GameState *game);

/* Estos dos van despues de renderGame() y antes del present. */
void renderReconnectOverlay(SDL_Renderer *renderer, TTF_Font *font);
void renderPauseMenu(SDL_Renderer *renderer, TTF_Font *font, int selected);

#endif
