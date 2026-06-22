#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H

#define MAX_PACKET_PLAYERS 4

/*
 * Roster de la sesion: que IPs tienen registrado cada id de jugador (1..4) y
 * quien es el host actual. Vive en el servidor (fuente de verdad) y cada
 * cliente guarda una copia local recibida por handshake / snapshot, para
 * poder decidir el siguiente host si la conexion se cae.
 */
typedef struct
{
    char ips[MAX_PACKET_PLAYERS][64];
    int slotUsed[MAX_PACKET_PLAYERS]; /* 1 si ese id ya fue asignado alguna vez */
    int currentHostId;                /* id del jugador cuya maquina corre el servidor */
    int registeredCount;              /* cuantos ids se han asignado en esta sesion */
} SessionRoster;

/* ---- Handshake (un solo intercambio fijo al abrir cada conexion TCP) ---- */
#define NET_MSG_JOIN_REQUEST  1
#define NET_MSG_JOIN_RESPONSE 2

typedef struct
{
    int type;        /* NET_MSG_JOIN_REQUEST */
    int preferredId; /* 0 = "soy nuevo"; 1..4 = intenta recuperar este id (rejoin) */
} JoinRequestPacket;

typedef struct
{
    int type;          /* NET_MSG_JOIN_RESPONSE */
    int assignedId;    /* 0 si el servidor rechazo la conexion (sala llena) */
    SessionRoster roster;
} JoinResponsePacket;

/* ---- Mensajes del loop de juego (un PlayerPacket por cada GamePacket) ---- */
typedef struct
{
    int playerId;
    float x;
    float y;
    float dirX;
    float dirY;
    int active;
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

/* El roster viaja embebido aqui: como el servidor ya responde un GamePacket
 * por cada PlayerPacket recibido (cada ~30ms), los cambios de roster (un
 * jugador se une o se cae) llegan a todos los clientes conectados sin
 * necesidad de un mensaje de broadcast aparte. */
typedef struct
{
    PlayerPacket players[MAX_PACKET_PLAYERS];
    BallPacket ball;
    int score[MAX_PACKET_PLAYERS];
    int winner; /* -1 si nadie ha ganado aun */
    SessionRoster roster;
} GamePacket;

#endif
