#include "../include/audio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GOAL_CHANNEL 0
#define KICK_CHANNEL 1
#define AMBIENT_CHANNEL 2

/* el ambiental suena bien de vez en cuando, cada 2 a 5 minutos */
#define AMBIENT_MIN_MS 120000
#define AMBIENT_MAX_MS 300000

/* musica (menu/in-game) y ambiental mas bajitas que los efectos (0-128) */
#define MUSIC_VOLUME 80
#define AMBIENT_VOLUME 35

static Mix_Chunk *goalSound = NULL;
static Mix_Chunk *kickSound = NULL;
static Mix_Chunk *ambientSound = NULL;
static Mix_Music *mainTheme = NULL;
static Mix_Music *ingameAmbience = NULL;

/* -1 = todavia no sincronizado con el primer snapshot recibido */
static int lastGoalCount = -1;
static int lastKickCount = -1;

static Uint32 nextAmbientAt = 0;
static int audioReady = 0; /* 1 solo si Mix_OpenAudio si abrio el dispositivo */

static Uint32 randomAmbientDelay(void)
{
    return AMBIENT_MIN_MS + (Uint32)(rand() % (AMBIENT_MAX_MS - AMBIENT_MIN_MS));
}

static Mix_Chunk *loadSound(const char *path)
{
    Mix_Chunk *chunk = Mix_LoadWAV(path);
    if (chunk == NULL)
        printf("No se pudo cargar %s: %s\n", path, Mix_GetError());
    return chunk;
}

void initAudio(void)
{
    srand((unsigned int)time(NULL));

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0)
    {
        printf("No se pudo abrir el audio: %s\n", Mix_GetError());
        return;
    }

    audioReady = 1;
    Mix_Init(MIX_INIT_MP3);

    goalSound = loadSound("assets/audio/gol1.mp3");
    kickSound = loadSound("assets/audio/kicking_sound.wav");
    ambientSound = loadSound("assets/audio/random1.mp3");
    if (ambientSound != NULL)
        Mix_VolumeChunk(ambientSound, AMBIENT_VOLUME);

    mainTheme = Mix_LoadMUS("assets/audio/main-theme.mp3");
    if (mainTheme == NULL)
        printf("No se pudo cargar assets/audio/main-theme.mp3: %s\n", Mix_GetError());
    else
    {
        Mix_VolumeMusic(MUSIC_VOLUME);
        Mix_PlayMusic(mainTheme, -1); /* -1 = bucle infinito */
    }

    ingameAmbience = Mix_LoadMUS("assets/audio/ambience-ingame.mp3");
    if (ingameAmbience == NULL)
        printf("No se pudo cargar assets/audio/ambience-ingame.mp3: %s\n", Mix_GetError());

    lastGoalCount = -1;
    lastKickCount = -1;
    nextAmbientAt = SDL_GetTicks() + randomAmbientDelay();
}

void freeAudio(void)
{
    if (!audioReady)
        return;

    if (goalSound != NULL)
        Mix_FreeChunk(goalSound);
    if (kickSound != NULL)
        Mix_FreeChunk(kickSound);
    if (ambientSound != NULL)
        Mix_FreeChunk(ambientSound);
    if (mainTheme != NULL || ingameAmbience != NULL)
        Mix_HaltMusic();
    if (mainTheme != NULL)
        Mix_FreeMusic(mainTheme);
    if (ingameAmbience != NULL)
        Mix_FreeMusic(ingameAmbience);

    goalSound = NULL;
    kickSound = NULL;
    ambientSound = NULL;
    mainTheme = NULL;
    ingameAmbience = NULL;

    Mix_CloseAudio();
    Mix_Quit();
    audioReady = 0;
}

void audioStopMainTheme(void)
{
    if (mainTheme != NULL)
        Mix_HaltMusic();
}

void audioStartIngameAmbience(void)
{
    if (ingameAmbience == NULL)
        return;

    Mix_VolumeMusic(MUSIC_VOLUME);
    Mix_PlayMusic(ingameAmbience, -1); /* -1 = bucle infinito */
}

void audioUpdate(int goalCount, int kickCount)
{
    if (lastGoalCount == -1)
        lastGoalCount = goalCount;
    else if (goalCount != lastGoalCount)
    {
        lastGoalCount = goalCount;

        /* si los goles vienen muy seguidos, no encimamos el sonido: solo
         * retumba si el anterior ya termino */
        if (goalSound != NULL && !Mix_Playing(GOAL_CHANNEL))
            Mix_PlayChannel(GOAL_CHANNEL, goalSound, 0);
    }

    if (lastKickCount == -1)
        lastKickCount = kickCount;
    else if (kickCount != lastKickCount)
    {
        lastKickCount = kickCount;

        if (kickSound != NULL)
            Mix_PlayChannel(KICK_CHANNEL, kickSound, 0);
    }

    Uint32 now = SDL_GetTicks();
    if (ambientSound != NULL && now >= nextAmbientAt)
    {
        Mix_PlayChannel(AMBIENT_CHANNEL, ambientSound, 0);
        nextAmbientAt = now + randomAmbientDelay();
    }
}
