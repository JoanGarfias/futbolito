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

/* Dibuja el frame del juego. NO presenta (no llama SDL_RenderPresent): el
 * llamador decide cuando presentar, para poder dibujar encima un overlay
 * (p.ej. "Reconectando...") antes del present. */
void renderGame(SDL_Renderer *renderer, TTF_Font *font, GameState *game);

/* Overlay semitransparente con texto centrado, para mostrar mientras se
 * espera la migracion de host. Dibujar despues de renderGame() y antes del
 * SDL_RenderPresent(). */
void renderReconnectOverlay(SDL_Renderer *renderer, TTF_Font *font);

#endif
