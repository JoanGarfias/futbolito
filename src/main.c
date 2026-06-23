#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
     *   futbolito --host        -> crea la sesion: arranca el servidor
     *                              embebido y se conecta a si mismo (id 1).
     *   futbolito <ip_del_host> -> se conecta a esa IP; el servidor le asigna
     *                              el primer id libre (2, 3, 4...).
     *   futbolito                -> equivale a "futbolito 127.0.0.1" (solo
     *                              cliente, util con futbolito_server aparte).
     * No hace falta pasar el numero de jugador ni la lista de las 4 IPs: el
     * servidor las asigna automaticamente (ver include/network.h).
     */
    const char *hostIp = "127.0.0.1";
    int isSessionHost = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--host") == 0)
            isSessionHost = 1;
        else
            hostIp = argv[i];
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

    /* Conexion persistente + hilo receptor. El id de jugador lo asigna el
     * servidor en el handshake, no se pasa por linea de comandos. */
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
         * seccion critica (el hilo receptor toca el mismo GameState). Sigue
         * funcionando aunque estemos reconectando: solo el envio al
         * servidor se pausa (netSendLocalPlayer no manda nada si no estamos
         * CONNECTED). */
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

        /* Migracion de host: no bloquea el bucle SDL. Si se pierde la
         * conexion, se lanza un hilo en segundo plano que reconecta solo;
         * mientras tanto se sigue dibujando (con un overlay) y respondiendo
         * a eventos (ESC sigue funcionando). */
        if (!netIsConnected(net) && netGetState(net) != NET_STATE_RECONNECTING)
            netBeginReconnect(net);

        if (netGetState(net) == NET_STATE_RECONNECTING)
        {
            netPollReconnect(net);
            renderReconnectOverlay(renderer, font);
        }

        SDL_RenderPresent(renderer);

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
