#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H

#define MAX_PACKET_PLAYERS 4

#define CHAT_MAX_LEN 48
#define CHAT_HISTORY 4

/* Quien tiene cada id de jugador (1..4) y quien es el host actual. Vive en
 * el servidor; cada cliente guarda su copia (llega por handshake/snapshot)
 * para poder elegir el siguiente host si se cae la conexion. */
typedef struct
{
    char ips[MAX_PACKET_PLAYERS][64];
    int slotUsed[MAX_PACKET_PLAYERS];   /* se reservo alguna vez (sirve para rejoin por IP) */
    int slotActive[MAX_PACKET_PLAYERS]; /* esta conectado AHORA, no solo registrado */
    int currentHostId;
    int registeredCount;
} SessionRoster;

/* Handshake: un solo intercambio al abrir cada conexion TCP. */
#define NET_MSG_JOIN_REQUEST  1
#define NET_MSG_JOIN_RESPONSE 2

typedef struct
{
    int type;
    int preferredId; /* 0 = nuevo; 1..4 = intenta recuperar ese id (rejoin) */
} JoinRequestPacket;

typedef struct
{
    int type;
    int assignedId; /* 0 = rechazado, sala llena */
    SessionRoster roster;
} JoinResponsePacket;

/* Loop del juego: un PlayerPacket por cada GamePacket de respuesta. */
typedef struct
{
    int playerId;
    float x;
    float y;
    float dirX;
    float dirY;
    int active;

    /* mensaje de chat saliente; chatSeq sube con cada mensaje nuevo para que
     * el host no lo procese dos veces (el texto puede venir vacio despues) */
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
    int playerId; /* 0 = vacio */
    char text[CHAT_MAX_LEN];
} ChatPacket;

/* El roster va embebido aqui porque el servidor ya responde un GamePacket
 * por cada PlayerPacket (~cada 30ms): asi los cambios de roster llegan solos,
 * sin necesitar un mensaje de broadcast aparte. */
typedef struct
{
    PlayerPacket players[MAX_PACKET_PLAYERS];
    BallPacket ball;
    int score[MAX_PACKET_PLAYERS];
    int winner; /* -1 = nadie ha ganado */
    ChatPacket chat[CHAT_HISTORY]; /* mas nuevo primero */
    SessionRoster roster;
} GamePacket;

#endif
