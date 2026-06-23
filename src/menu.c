#include "../include/menu.h"
#include "../include/render.h"
#include <string.h>
#include <stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define OPTION_HOST 0
#define OPTION_JOIN 1
#define OPTION_QUIT 2
#define OPTION_COUNT 3

#define IP_INPUT_MAX 64

static const char *OPTION_LABELS[OPTION_COUNT] = {
    "Crear partida (host)",
    "Unirse a una partida",
    "Salir"};

/* Mismo estilo que renderReconnectOverlay() en render.c: cuadro negro
 * semitransparente con borde blanco. */
static void renderPanel(SDL_Renderer *renderer, SDL_Rect panel)
{
    SDL_BlendMode prevMode;
    SDL_GetRenderDrawBlendMode(renderer, &prevMode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawBlendMode(renderer, prevMode);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &panel);
}

static void renderMainScreen(SDL_Renderer *renderer, TTF_Font *font, int selected)
{
    SDL_SetRenderDrawColor(renderer, 30, 110, 45, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};

    SDL_Rect panel = {SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 150, 400, 300};
    renderPanel(renderer, panel);

    renderText(renderer, font, "FUTBOLITO", panel.x + 130, panel.y + 25, white);

    for (int i = 0; i < OPTION_COUNT; i++)
    {
        char label[40];
        snprintf(label, sizeof(label), "%s%s", (i == selected) ? "> " : "  ", OPTION_LABELS[i]);
        renderText(renderer, font, label, panel.x + 50, panel.y + 100 + i * 40, white);
    }

    renderText(renderer, font, "Flechas o W/S, Enter para elegir",
               panel.x + 50, panel.y + 250, white);

    SDL_RenderPresent(renderer);
}

static void renderJoinScreen(SDL_Renderer *renderer, TTF_Font *font, const char *ipInput)
{
    SDL_SetRenderDrawColor(renderer, 30, 110, 45, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};

    SDL_Rect panel = {SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 100, 400, 200};
    renderPanel(renderer, panel);

    renderText(renderer, font, "IP del host:", panel.x + 40, panel.y + 25, white);

    SDL_Rect box = {panel.x + 40, panel.y + 65, panel.w - 80, 32};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &box);

    int blink = (SDL_GetTicks() / 500) % 2 == 0;
    char line[IP_INPUT_MAX + 2];
    snprintf(line, sizeof(line), "%s%s", ipInput, blink ? "_" : "");
    renderText(renderer, font, line, box.x + 8, box.y + 5, white);

    renderText(renderer, font, "Enter para conectar, Esc para volver",
               panel.x + 40, panel.y + 130, white);

    SDL_RenderPresent(renderer);
}

MenuAction runMenu(SDL_Renderer *renderer, TTF_Font *font, char *outIp, size_t outIpSize)
{
    int selected = OPTION_HOST;
    int onJoinScreen = 0;
    char ipInput[IP_INPUT_MAX] = "127.0.0.1";

    SDL_StopTextInput();

    SDL_Event event;

    for (;;)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return MENU_ACTION_QUIT;

            if (!onJoinScreen)
            {
                if (event.type != SDL_KEYDOWN)
                    continue;

                SDL_Keycode key = event.key.keysym.sym;

                if (key == SDLK_UP || key == SDLK_w)
                    selected = (selected + OPTION_COUNT - 1) % OPTION_COUNT;
                else if (key == SDLK_DOWN || key == SDLK_s)
                    selected = (selected + 1) % OPTION_COUNT;
                else if (key == SDLK_ESCAPE)
                    return MENU_ACTION_QUIT;
                else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
                {
                    if (selected == OPTION_HOST)
                    {
                        strncpy(outIp, "127.0.0.1", outIpSize - 1);
                        outIp[outIpSize - 1] = '\0';
                        return MENU_ACTION_HOST;
                    }

                    if (selected == OPTION_JOIN)
                    {
                        onJoinScreen = 1;
                        SDL_StartTextInput();
                        continue;
                    }

                    /* selected == OPTION_QUIT */
                    return MENU_ACTION_QUIT;
                }

                continue;
            }

            /* Pantalla de "unirse": escribir la IP. */
            if (event.type == SDL_TEXTINPUT)
            {
                size_t len = strlen(ipInput);
                size_t add = strlen(event.text.text);

                if (len + add < IP_INPUT_MAX - 1)
                    strcat(ipInput, event.text.text);

                continue;
            }

            if (event.type != SDL_KEYDOWN)
                continue;

            SDL_Keycode key = event.key.keysym.sym;

            if (key == SDLK_BACKSPACE)
            {
                size_t len = strlen(ipInput);
                if (len > 0)
                    ipInput[len - 1] = '\0';
            }
            else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
            {
                if (ipInput[0] != '\0')
                {
                    strncpy(outIp, ipInput, outIpSize - 1);
                    outIp[outIpSize - 1] = '\0';
                    SDL_StopTextInput();
                    return MENU_ACTION_JOIN;
                }
            }
            else if (key == SDLK_ESCAPE)
            {
                onJoinScreen = 0;
                SDL_StopTextInput();
            }
        }

        if (onJoinScreen)
            renderJoinScreen(renderer, font, ipInput);
        else
            renderMainScreen(renderer, font, selected);

        SDL_Delay(16);
    }
}
