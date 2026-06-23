#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "game.h"

void handleInput(GameState *game, int localPlayerId);
void handleKeyDown(GameState *game, SDL_Keycode key);

/* Chat de texto: procesa un evento de SDL (tecla T para abrir, texto
 * tecleado, Enter para enviar, Esc para cancelar, Backspace para borrar). */
void handleChatEvent(GameState *game, SDL_Event *event);

#endif