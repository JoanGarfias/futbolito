#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/game.h"
#include "../include/input.h"
#include "../include/render.h"
#include "../include/network.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define SEND_INTERVAL_MS 30

int main(int argc, char *argv[])
{
    /*
     * Uso:
     *   futbolito <id>                        -> cliente local (127.0.0.1)
     *   futbolito <id> <ipServidor>           -> cliente puro a un servidor fijo
     *   futbolito <id> <ip1> <ip2> <ip3> <ip4> -> modo punto a punto (4 equipos
     *                                             con migracion de host)
     */
    int localPlayerId = 1;
    const char *peers[4] = {"127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1"};
    int peerCount = 1;

    if (argc >= 2)
        localPlayerId = atoi(argv[1]);

    if (argc >= 3)
    {
        peerCount = argc - 2;
        if (peerCount > 4)
            peerCount = 4;
        for (int i = 0; i < peerCount; i++)
            peers[i] = argv[2 + i];
    }

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
        "FUTBOLITO - Red",
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

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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

    loadSprites(renderer);

    GameState game;
    initGame(&game);

    /* Conexion persistente + hilo receptor. */
    NetClient *net = netConnect(peers, peerCount, localPlayerId, &game);

    if (net == NULL)
    {
        printf("No se pudo iniciar la red. Saliendo.\n");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Event event;
    Uint32 lastSend = 0;

    while (game.running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                game.running = 0;

            /* Si el chat ya estaba abierto, Esc lo cierra (no cierra el juego). */
            int chatWasOpen = game.chatOpen;

            handleChatEvent(&game, &event);

            if (!chatWasOpen && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                game.running = 0;
        }

        /* Input: escribe la posicion del jugador local dentro de la
         * seccion critica (el hilo receptor toca el mismo GameState). */
        netLockState(net);
        handleInput(&game, localPlayerId);
        netUnlockState(net);

        Uint32 now = SDL_GetTicks();
        if (now - lastSend >= SEND_INTERVAL_MS)
        {
            netSendLocalPlayer(net, &game);
            lastSend = now;
        }

        /* Render: lee el estado dentro de la seccion critica. */
        netLockState(net);
        renderGame(renderer, font, &game);
        netUnlockState(net);

        if (!netIsConnected(net))
        {
            printf("[CLIENTE] Conexion perdida. Intentando migrar de host...\n");
            if (!netReconnect(net))
            {
                printf("[CLIENTE] No hay ningun host disponible. Saliendo.\n");
                game.running = 0;
            }
            else
            {
                printf("[CLIENTE] Reconectado. Host actual: jugador %d\n",
                       netCurrentHost(net));
            }
        }

        SDL_Delay(16);
    }

    netDisconnect(net);

    freeSprites();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();

    return 0;
}
