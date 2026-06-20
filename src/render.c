#include "../include/render.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

/* ----- Funciones de ayuda para dibujar la cancha ----- */

/* Contorno de circulo (algoritmo de punto medio). */
static void drawCircleOutline(SDL_Renderer *r, int cx, int cy, int radius)
{
    int x = radius, y = 0, err = 0;
    while (x >= y)
    {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);
        y++;
        if (err <= 0)
            err += 2 * y + 1;
        if (err > 0)
        {
            x--;
            err -= 2 * x + 1;
        }
    }
}

/* Circulo con grosor de 2 px. */
static void drawCircle(SDL_Renderer *r, int cx, int cy, int radius)
{
    drawCircleOutline(r, cx, cy, radius);
    drawCircleOutline(r, cx, cy, radius - 1);
}

/* Circulo relleno (para los puntos de la cancha). */
static void fillCircle(SDL_Renderer *r, int cx, int cy, int radius)
{
    for (int dy = -radius; dy <= radius; dy++)
    {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

/* Arco entre dos angulos (radianes), con grosor de 2 px. */
static void drawArc(SDL_Renderer *r, int cx, int cy, int radius, double a0, double a1)
{
    for (double a = a0; a <= a1; a += 0.008)
    {
        SDL_RenderDrawPoint(r, cx + (int)(radius * cos(a)), cy + (int)(radius * sin(a)));
        SDL_RenderDrawPoint(r, cx + (int)((radius - 1) * cos(a)), cy + (int)((radius - 1) * sin(a)));
    }
}

/* Rectangulo con grosor de 2 px. */
static void drawRect2(SDL_Renderer *r, SDL_Rect rect)
{
    SDL_RenderDrawRect(r, &rect);
    SDL_Rect inner = {rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2};
    SDL_RenderDrawRect(r, &inner);
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

    // Césped con franjas verticales
    SDL_SetRenderDrawColor(renderer, 62, 158, 68, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 54, 146, 60, 255);
    for (int i = 0; i < 9; i++)
    {
        if (i % 2 == 1)
        {
            SDL_Rect stripe = {50 + i * 100, 50, 100, 500};
            SDL_RenderFillRect(renderer, &stripe);
        }
    }

    // Líneas blancas de la cancha
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect borde = {50, 50, 900, 500};
    drawRect2(renderer, borde);

    // Línea central
    SDL_RenderDrawLine(renderer, 500, 50, 500, 550);
    SDL_RenderDrawLine(renderer, 501, 50, 501, 550);

    // Círculo y punto central
    drawCircle(renderer, 500, 300, 72);
    fillCircle(renderer, 500, 300, 4);

    // Área grande, área chica, punto y arco de penal (izquierda)
    SDL_Rect penL = {50, 170, 120, 260};
    drawRect2(renderer, penL);
    SDL_Rect areaChicaL = {50, 240, 45, 120};
    drawRect2(renderer, areaChicaL);
    fillCircle(renderer, 135, 300, 3);
    drawArc(renderer, 135, 300, 55, -0.88, 0.88);

    // Área grande, área chica, punto y arco de penal (derecha)
    SDL_Rect penR = {830, 170, 120, 260};
    drawRect2(renderer, penR);
    SDL_Rect areaChicaR = {905, 240, 45, 120};
    drawRect2(renderer, areaChicaR);
    fillCircle(renderer, 865, 300, 3);
    drawArc(renderer, 865, 300, 55, M_PI - 0.88, M_PI + 0.88);

    // Arcos de esquina
    drawArc(renderer, 50, 50, 14, 0.0, M_PI / 2);
    drawArc(renderer, 950, 50, 14, M_PI / 2, M_PI);
    drawArc(renderer, 50, 550, 14, 3.0 * M_PI / 2, 2.0 * M_PI);
    drawArc(renderer, 950, 550, 14, M_PI, 3.0 * M_PI / 2);

    // Porterías según jugadores conectados
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

    // Cartel de ganador
    if (game->winner >= 0 && game->winner < MAX_PLAYERS)
    {
        SDL_Rect panel = {SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 40, 400, 80};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
        SDL_RenderFillRect(renderer, &panel);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &panel);

        char winText[40];
        snprintf(winText, sizeof(winText), "GANA EL JUGADOR %d", game->winner + 1);

        SDL_Color color = playerColor(game->winner);
        renderText(renderer, font, winText, SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT / 2 - 25, color);
        renderText(renderer, font, "Nueva partida en unos segundos...",
                   SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 5, white);
    }

    SDL_RenderPresent(renderer);
}
