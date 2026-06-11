#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

#include "../include/game.h"
#include "../include/input.h"
#include "../include/physics.h"
#include "../include/render.h"
#include "../include/network.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Error al iniciar SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0)
    {
        printf("Error al iniciar SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "FUTBOLITO - Version Red",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (window == NULL)
    {
        printf("Error al crear ventana: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);

    if (renderer == NULL)
    {
        printf("Error al crear renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("assets/fonts/arial.ttf", 20);

    if (font == NULL)
    {
        printf("Error al cargar fuente: %s\n", TTF_GetError());

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        TTF_Quit();
        SDL_Quit();

        return 1;
    }

    GameState game;
    initGame(&game);

    NetworkState network;
    initNetworkState(&network, 1);

    printNetworkState(&network);

    const char *serverIp = "127.0.0.1";
    int localPlayerId = 1;

    if (!initNetworkClient(serverIp))
    {
        printf("No se pudo iniciar cliente de red\n");
    }

    SDL_Event event;

    Uint32 lastNetworkUpdate = 0;

    while (game.running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                game.running = 0;
            }

            if (event.type == SDL_KEYDOWN)
            {
                handleKeyDown(&game, event.key.keysym.sym);

                syncNetworkFromGame(&network, &game);
                electNewHost(&network);

                if (event.key.keysym.sym == SDLK_5)
                {
                    printNetworkState(&network);
                }
            }
        }

        handleInput(&game);

        updatePhysics(&game);

        syncNetworkFromGame(&network, &game);

        Uint32 now = SDL_GetTicks();

        if (now - lastNetworkUpdate >= 250)
        {
            sendLocalPlayerAndReceiveState(
                &game,
                localPlayerId,
                serverIp);

            lastNetworkUpdate = now;
        }

        renderGame(renderer, font, &game);

        SDL_Delay(16);
    }

    shutdownNetworkClient();

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();

    return 0;
}