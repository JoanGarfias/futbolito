#include "../include/audio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GOAL_CHANNEL 0
#define KICK_CHANNEL 1
#define AMBIENT_CHANNEL 2

/* el ambiental suena cada tanto, en un rango random de 25 a 60 segundos */
#define AMBIENT_MIN_MS 25000
#define AMBIENT_MAX_MS 60000

static Mix_Chunk *goalSound = NULL;
static Mix_Chunk *kickSound = NULL;
static Mix_Chunk *ambientSound = NULL;

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

    goalSound = NULL;
    kickSound = NULL;
    ambientSound = NULL;

    Mix_CloseAudio();
    Mix_Quit();
    audioReady = 0;
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
