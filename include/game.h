#ifndef GAME_H
#define GAME_H

#include "player.h"
#include "ball.h"

#define MAX_PLAYERS 4

/* Goles necesarios para ganar la partida. */
#define GOALS_TO_WIN 5

/* Chat de texto: se abre con la tecla T. */
#define CHAT_MAX_LEN 48
#define CHAT_HISTORY 4

typedef struct
{
    int playerId; /* 0 = entrada vacia */
    char text[CHAT_MAX_LEN];
} ChatMessage;

typedef struct
{
    int running;
    int score[MAX_PLAYERS];
    int winner; /* -1 si nadie ha ganado; si no, indice del jugador ganador */

    Player players[MAX_PLAYERS];
    Ball ball;

    /* Chat: historial recibido del host (mas nuevo primero). */
    ChatMessage chatLog[CHAT_HISTORY];

    /* Chat: estado LOCAL de la caja de texto (no lo toca el hilo receptor). */
    int chatOpen;                 /* 1 mientras se esta escribiendo (tecla T) */
    char chatInput[CHAT_MAX_LEN]; /* texto que se va escribiendo */
    char pendingChatMsg[CHAT_MAX_LEN]; /* listo para viajar en el proximo envio */
    int localChatSeq;             /* sube cada vez que se envia un mensaje */

} GameState;

void initGame(GameState *game);
void resetBall(GameState *game);

#endif