#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H

#define MAX_PACKET_PLAYERS 4

#define CHAT_MAX_LEN 48
#define CHAT_HISTORY 4

typedef struct
{
    int playerId;
    float x;
    float y;
    float dirX;
    float dirY;
    int active;

    /* Chat: mensaje "saliente" del jugador local. chatSeq sube cada vez que
     * se escribe un mensaje nuevo; el host lo usa para no reenviarlo dos
     * veces (el texto puede llegar vacio en los paquetes siguientes). */
    char chatMsg[CHAT_MAX_LEN];
    int chatSeq;
} PlayerPacket;

typedef struct
{
    float x;
    float y;
    float vx;
    float vy;
    int size;
    int lastPlayerTouched;
} BallPacket;

typedef struct
{
    int playerId; /* 0 = entrada vacia */
    char text[CHAT_MAX_LEN];
} ChatPacket;

typedef struct
{
    PlayerPacket players[MAX_PACKET_PLAYERS];
    BallPacket ball;
    int score[MAX_PACKET_PLAYERS];
    int winner; /* -1 si nadie ha ganado aun */
    ChatPacket chat[CHAT_HISTORY]; /* ultimos mensajes, mas nuevo primero */
} GamePacket;

#endif