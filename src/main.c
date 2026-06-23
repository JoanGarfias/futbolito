#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/game.h"
#include "../include/input.h"
#include "../include/render.h"
#include "../include/network.h"
#include "../include/menu.h"
#include "../include/audio.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define SEND_INTERVAL_MS 30

int main(int argc, char *argv[])
{
    /* sin argumentos abre el menu (crear/unirse/salir); con "--host" o una
     * IP se salta el menu y conecta directo, por si quieren probar rapido
     * desde la terminal */
    char hostIp[64] = "127.0.0.1";
    int isSessionHost = 0;
    int skipMenu = 0; /* 1 si se paso algun argumento: nos saltamos el menu */

    for (int i = 1; i < argc; i++)
    {
        skipMenu = 1;

        if (strcmp(argv[i], "--host") == 0)
            isSessionHost = 1;
        else
        {
            strncpy(hostIp, argv[i], sizeof(hostIp) - 1);
            hostIp[sizeof(hostIp) - 1] = '\0';
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
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

    /* Si no se paso ningun argumento, mostramos el menu de inicio para
     * elegir crear partida / unirse / salir, en vez de obligar a usar la
     * terminal. */
    if (!skipMenu)
    {
        MenuAction action = runMenu(renderer, font, hostIp, sizeof(hostIp));

        if (action == MENU_ACTION_QUIT)
        {
            TTF_CloseFont(font);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return 0;
        }

        isSessionHost = (action == MENU_ACTION_HOST);
    }

    loadSprites(renderer);
    initAudio();

    GameState game;
    initGame(&game);

    NetClient *net = netConnect(hostIp, isSessionHost, &game);

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

    int localPlayerId = netGetMyId(net);

    SDL_Event event;
    Uint32 lastSend = 0;

    /* Menu de pausa: Esc ya NO cierra el juego directo, abre este menu
     * (Continuar/Salir) para confirmar primero. */
    int paused = 0;
    int pauseSelected = 0; /* 0 = Continuar, 1 = Salir */

    while (game.running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                game.running = 0;

            if (paused)
            {
                if (event.type == SDL_KEYDOWN)
                {
                    SDL_Keycode key = event.key.keysym.sym;

                    if (key == SDLK_UP || key == SDLK_w || key == SDLK_DOWN || key == SDLK_s)
                        pauseSelected = 1 - pauseSelected;
                    else if (key == SDLK_ESCAPE)
                        paused = 0; /* Esc en el menu de pausa = continuar */
                    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
                    {
                        if (pauseSelected == 1)
                            game.running = 0;
                        else
                            paused = 0;
                    }
                }

                continue;
            }

            /* Si el chat ya estaba abierto, Esc lo cierra (no abre la pausa). */
            int chatWasOpen = game.chatOpen;

            handleChatEvent(&game, &event);

            if (!chatWasOpen && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                paused = 1;
                pauseSelected = 0;
            }
        }

        /* el receptor toca el mismo GameState, por eso el lock */
        netLockState(net);
        if (!paused)
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
        int goalCount = game.goalCount;
        int kickCount = game.ball.kickCount;
        netUnlockState(net);

        audioUpdate(goalCount, kickCount);

        /* si se cae la conexion, esto lanza la migracion en segundo plano
         * sin trabar la ventana */
        if (!netIsConnected(net) && netGetState(net) != NET_STATE_RECONNECTING)
            netBeginReconnect(net);

        if (netGetState(net) == NET_STATE_RECONNECTING)
        {
            netPollReconnect(net);
            renderReconnectOverlay(renderer, font);
        }

        if (paused)
            renderPauseMenu(renderer, font, pauseSelected);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    netDisconnect(net);

    freeSprites();
    freeAudio();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();

    return 0;
}
