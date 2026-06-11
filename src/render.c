#include "../include/render.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

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

            setPlayerColor(renderer, i);
            SDL_RenderFillRect(renderer, &playerRect);
        }
    }

    // Balón
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect ballRect = {
        (int)game->ball.x,
        (int)game->ball.y,
        game->ball.size,
        game->ball.size};

    SDL_RenderFillRect(renderer, &ballRect);

    SDL_RenderPresent(renderer);
}