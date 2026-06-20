#include "../include/render.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

/* Cada frame del sprite mide 24x24 px (exportado de Aseprite). */
#define FRAME_W 24
#define FRAME_H 24

/* Direcciones a las que puede mirar el personaje. */
#define FACE_DOWN 0
#define FACE_UP 1
#define FACE_SIDE 2
#define FACE_COUNT 3

/* Una animacion = una tira horizontal de frames. */
typedef struct
{
    SDL_Texture *tex;
    int frames;
} Anim;

static Anim animIdle[FACE_COUNT]; /* quieto: abajo, arriba, lado */
static Anim animWalk[FACE_COUNT]; /* caminando: abajo, arriba, lado */

/* Estado de animacion por jugador (lo lleva el render, sirve para locales y
 * remotos porque deducimos el movimiento comparando posiciones). */
static float prevX[MAX_PLAYERS];
static float prevY[MAX_PLAYERS];
static int facing[MAX_PLAYERS];   /* FACE_DOWN / FACE_UP / FACE_SIDE */
static int faceLeft[MAX_PLAYERS]; /* 1 si mira a la izquierda (espejo) */

/* Carga una tira de animacion (PNG con transparencia). */
static Anim loadAnim(SDL_Renderer *renderer, const char *path)
{
    Anim a = {NULL, 0};

    SDL_Surface *surface = IMG_Load(path);
    if (surface == NULL)
        return a; /* sin sprite: se usara el color de respaldo */

    a.tex = SDL_CreateTextureFromSurface(renderer, surface);
    a.frames = surface->w / FRAME_W;
    if (a.frames < 1)
        a.frames = 1;

    SDL_FreeSurface(surface);
    return a;
}

void loadSprites(SDL_Renderer *renderer)
{
    IMG_Init(IMG_INIT_PNG);

    animIdle[FACE_DOWN] = loadAnim(renderer, "assets/sprites/idle_down.png");
    animIdle[FACE_UP] = loadAnim(renderer, "assets/sprites/idle_up.png");
    animIdle[FACE_SIDE] = loadAnim(renderer, "assets/sprites/idle_side.png");

    animWalk[FACE_DOWN] = loadAnim(renderer, "assets/sprites/walk_down.png");
    animWalk[FACE_UP] = loadAnim(renderer, "assets/sprites/walk_up.png");
    animWalk[FACE_SIDE] = loadAnim(renderer, "assets/sprites/walk_side.png");
}

void freeSprites(void)
{
    for (int i = 0; i < FACE_COUNT; i++)
    {
        if (animIdle[i].tex)
            SDL_DestroyTexture(animIdle[i].tex);
        if (animWalk[i].tex)
            SDL_DestroyTexture(animWalk[i].tex);
        animIdle[i].tex = NULL;
        animWalk[i].tex = NULL;
    }

    IMG_Quit();
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

/* Color identificador de cada jugador (tambien tine el sprite). */
static SDL_Color playerColor(int playerIndex)
{
    SDL_Color c = {255, 255, 255, 255};

    if (playerIndex == 0)
    {
        c.r = 80;  c.g = 150; c.b = 255; /* Azul */
    }
    else if (playerIndex == 1)
    {
        c.r = 255; c.g = 90;  c.b = 90; /* Rojo */
    }
    else if (playerIndex == 2)
    {
        c.r = 255; c.g = 220; c.b = 60; /* Amarillo */
    }
    else if (playerIndex == 3)
    {
        c.r = 90;  c.g = 220; c.b = 120; /* Verde */
    }

    return c;
}

static void setPlayerColor(SDL_Renderer *renderer, int playerIndex)
{
    SDL_Color c = playerColor(playerIndex);
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
}

/* Actualiza la direccion y el estado (caminando/quieto) de un jugador
 * comparando su posicion actual con la del frame anterior. */
static int updateFacing(int i, float x, float y)
{
    float dx = x - prevX[i];
    float dy = y - prevY[i];
    prevX[i] = x;
    prevY[i] = y;

    int moving = (fabsf(dx) > 0.1f || fabsf(dy) > 0.1f);

    if (moving)
    {
        if (fabsf(dx) >= fabsf(dy))
        {
            facing[i] = FACE_SIDE;
            faceLeft[i] = (dx < 0);
        }
        else
        {
            facing[i] = (dy < 0) ? FACE_UP : FACE_DOWN;
        }
    }

    return moving;
}

static void drawPlayerSprite(SDL_Renderer *renderer, int i, SDL_Rect dst, int moving, Uint32 ticks)
{
    Anim a = moving ? animWalk[facing[i]] : animIdle[facing[i]];
    if (a.tex == NULL)
        a = animIdle[FACE_DOWN]; /* respaldo: alguna direccion no cargo */

    if (a.tex == NULL)
    {
        setPlayerColor(renderer, i);
        SDL_RenderFillRect(renderer, &dst);
        return;
    }

    int interval = moving ? 120 : 250; /* ms por frame */
    int frame = (ticks / interval) % a.frames;

    SDL_Rect src = {frame * FRAME_W, 0, FRAME_W, FRAME_H};

    /* Tinte con el color del jugador (la playera real se cambia despues). */
    SDL_Color c = playerColor(i);
    SDL_SetTextureColorMod(a.tex, c.r, c.g, c.b);

    SDL_RendererFlip flip = faceLeft[i] ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, a.tex, &src, &dst, 0.0, NULL, flip);

    SDL_SetTextureColorMod(a.tex, 255, 255, 255);
}

void renderGame(SDL_Renderer *renderer, TTF_Font *font, GameState *game)
{
    /* La primera vez, fijamos las posiciones previas para no detectar un
     * "salto" inicial falso. */
    static int prevInit = 0;
    if (!prevInit)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            prevX[i] = game->players[i].x;
            prevY[i] = game->players[i].y;
            facing[i] = FACE_DOWN;
        }
        prevInit = 1;
    }

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

    // Jugadores (sprite animado con direccion, o cuadro de color de respaldo)
    Uint32 ticks = SDL_GetTicks();

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!game->players[i].active)
            continue;

        int moving = updateFacing(i, game->players[i].x, game->players[i].y);

        /* Dibujamos el sprite un poco mas grande que la ficha para que se vea
         * mejor, centrado sobre la posicion del jugador. */
        int w = game->players[i].width;
        int h = game->players[i].height;
        int drawW = w + 14;
        int drawH = h + 14;

        SDL_Rect dst = {
            (int)game->players[i].x - (drawW - w) / 2,
            (int)game->players[i].y - (drawH - h) / 2,
            drawW,
            drawH};

        drawPlayerSprite(renderer, i, dst, moving, ticks);
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
