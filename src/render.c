#include "../include/render.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

/* Texturas de los sprites. NULL = no hay sprite, se usa el color de respaldo.
 * Indices 0-3 = jugadores, indice 4 (BALL_SPRITE) = pelota. */
#define BALL_SPRITE MAX_PLAYERS
static SDL_Texture *sprites[MAX_PLAYERS + 1] = {NULL};

/* Carga un BMP y lo convierte en textura, usando magenta (255,0,255) como
 * color transparente. Devuelve NULL si el archivo no existe. */
static SDL_Texture *loadBMP(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = SDL_LoadBMP(path);

    if (surface == NULL)
        return NULL; /* sin sprite: se usara el color de respaldo */

    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 0, 255));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

void loadSprites(SDL_Renderer *renderer)
{
    sprites[0] = loadBMP(renderer, "assets/sprites/player1.bmp");
    sprites[1] = loadBMP(renderer, "assets/sprites/player2.bmp");
    sprites[2] = loadBMP(renderer, "assets/sprites/player3.bmp");
    sprites[3] = loadBMP(renderer, "assets/sprites/player4.bmp");
    sprites[BALL_SPRITE] = loadBMP(renderer, "assets/sprites/ball.bmp");
}

void freeSprites(void)
{
    for (int i = 0; i <= MAX_PLAYERS; i++)
    {
        if (sprites[i] != NULL)
        {
            SDL_DestroyTexture(sprites[i]);
            sprites[i] = NULL;
        }
    }
}

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);

    if (surface == NULL)
    {
        printf("Error creando surface de texto: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (texture == NULL)
    {
        printf("Error creando textura de texto: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dest = {x, y, surface->w, surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &dest);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void setPlayerColor(SDL_Renderer *renderer, int playerIndex)
{
    if (playerIndex == 0)
        SDL_SetRenderDrawColor(renderer, 0, 120, 255, 255);      // Azul
    else if (playerIndex == 1)
        SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);      // Rojo
    else if (playerIndex == 2)
        SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);      // Amarillo
    else if (playerIndex == 3)
        SDL_SetRenderDrawColor(renderer, 0, 220, 100, 255);      // Verde
}

void renderGame(SDL_Renderer *renderer, TTF_Font *font, GameState *game)
{
    SDL_SetRenderDrawColor(renderer, 30, 140, 60, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Borde del campo
    SDL_Rect campo = {50, 50, 900, 500};
    SDL_RenderDrawRect(renderer, &campo);

    // Línea central
    SDL_RenderDrawLine(renderer, SCREEN_WIDTH / 2, 50, SCREEN_WIDTH / 2, 550);

    // Porterías activas por jugador
    if (game->players[0].active)
    {
        SDL_Rect goalLeft = {30, 250, 20, 100};
        SDL_RenderDrawRect(renderer, &goalLeft);
    }

    if (game->players[1].active)
    {
        SDL_Rect goalRight = {950, 250, 20, 100};
        SDL_RenderDrawRect(renderer, &goalRight);
    }

    if (game->players[2].active)
    {
        SDL_Rect goalTop = {450, 30, 100, 20};
        SDL_RenderDrawRect(renderer, &goalTop);
    }

    if (game->players[3].active)
    {
        SDL_Rect goalBottom = {450, 550, 100, 20};
        SDL_RenderDrawRect(renderer, &goalBottom);
    }

    // Centro del campo
    SDL_Rect centro = {SCREEN_WIDTH / 2 - 5, SCREEN_HEIGHT / 2 - 5, 10, 10};
    SDL_RenderFillRect(renderer, &centro);

    // Marcador pequeño con color del jugador
    char scoreText[20];
    SDL_Color white = {255, 255, 255, 255};

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!game->players[i].active)
            continue;

        int y = 10 + (i * 25);

        setPlayerColor(renderer, i);

        SDL_Rect colorBox = {15, y + 7, 10, 10};
        SDL_RenderFillRect(renderer, &colorBox);

        snprintf(scoreText, sizeof(scoreText), "P%d: %d", i + 1, game->score[i]);
        renderText(renderer, font, scoreText, 32, y, white);
    }

    // Jugadores
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (game->players[i].active)
        {
            SDL_Rect playerRect = {
                (int)game->players[i].x,
                (int)game->players[i].y,
                game->players[i].width,
                game->players[i].height};

            if (sprites[i] != NULL)
            {
                SDL_RenderCopy(renderer, sprites[i], NULL, &playerRect);
            }
            else
            {
                setPlayerColor(renderer, i);
                SDL_RenderFillRect(renderer, &playerRect);
            }
        }
    }

    // Balón
    SDL_Rect ballRect = {
        (int)game->ball.x,
        (int)game->ball.y,
        game->ball.size,
        game->ball.size};

    if (sprites[BALL_SPRITE] != NULL)
    {
        SDL_RenderCopy(renderer, sprites[BALL_SPRITE], NULL, &ballRect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &ballRect);
    }

    SDL_RenderPresent(renderer);
}