#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stddef.h>

/* Menu de inicio, antes de conectar a la red: crear partida (host), unirse
 * escribiendo la IP, o salir. Asi ya no es obligatorio usar argumentos de
 * linea de comandos. */

typedef enum
{
    MENU_ACTION_HOST,
    MENU_ACTION_JOIN,
    MENU_ACTION_QUIT
} MenuAction;

/* Bloquea con su propio loop de eventos/dibujo hasta que el usuario elige.
 * Si la accion es JOIN, deja la IP en outIp (tamano outIpSize). Tambien
 * regresa QUIT si cierran la ventana o le dan Esc en la pantalla principal. */
MenuAction runMenu(SDL_Renderer *renderer, TTF_Font *font, char *outIp, size_t outIpSize);

#endif
