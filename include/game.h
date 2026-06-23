#ifndef GAME_H
#define GAME_H

#include "player.h"
#include "ball.h"

#define MAX_PLAYERS 4
#define GOALS_TO_WIN 5

#define CHAT_MAX_LEN 48
#define CHAT_HISTORY 4

typedef struct
{
    int playerId; /* 0 = vacio */
    char text[CHAT_MAX_LEN];
} ChatMessage;

typedef struct
{
    int running;
    int score[MAX_PLAYERS];
    int winner; /* -1 si nadie ha ganado todavia */
    int goalCount; /* sube con cada gol (para el sonido), nunca se reinicia */

    Player players[MAX_PLAYERS];
    Ball ball;

    ChatMessage chatLog[CHAT_HISTORY]; /* historial recibido del host, mas nuevo primero */

    /* caja de chat local (tecla T); el hilo receptor no toca estos campos */
    int chatOpen;
    char chatInput[CHAT_MAX_LEN];
    char pendingChatMsg[CHAT_MAX_LEN]; /* listo para salir en el proximo envio */
    int localChatSeq;                  /* sube con cada mensaje nuevo */

} GameState;

void initGame(GameState *game);
void resetBall(GameState *game);

#endif
