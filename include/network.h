#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"

/*
 * CLIENTE de red del Futbolito (con migracion de host / punto a punto).
 *
 * NetClient es un tipo OPACO: su definicion real (socket, mutex, hilo) vive en
 * network.c. Asi este header NO arrastra winsock2.h/windows.h, evitando
 * conflictos con SDL en main.c.
 *
 * Dos modos segun cuantas IPs reciba:
 *   - 1 IP  -> cliente puro: se conecta a ese servidor; si se cae, termina.
 *   - 4 IPs -> modo punto a punto: el host inicial es el jugador 1. Si el host
 *              cae, todos calculan el mismo siguiente id (host+1, con wrap a
 *              1) de forma deterministica. SOLO ese equipo arranca el
 *              servidor embebido (serverStart()); todos los demas unicamente
 *              reintentan connect() contra esa IP, nunca arrancan servidor
 *              propio.
 *
 * El hilo principal (SDL) y el hilo receptor acceden al mismo GameState, por
 * eso TODO acceso debe ir entre netLockState()/netUnlockState() (seccion
 * critica).
 */

typedef struct NetClient NetClient;

/* Conecta usando la lista de IPs de los equipos (peers[0..peerCount-1]).
 * myId es el id del jugador local (1..4). Devuelve NULL si no logra conectar. */
NetClient *netConnect(const char *peers[], int peerCount, int myId, GameState *game);

/* Envia la posicion del jugador local al host actual. */
void netSendLocalPlayer(NetClient *nc, GameState *game);

/* Entra/sale de la seccion critica que protege el GameState compartido. */
void netLockState(NetClient *nc);
void netUnlockState(NetClient *nc);

/* 1 si la conexion sigue viva, 0 si se perdio. */
int netIsConnected(NetClient *nc);

/* Tras perder la conexion: elige el siguiente host y se reconecta (migracion).
 * Devuelve 1 si logro reconectar, 0 si ya no hay host disponible. */
int netReconnect(NetClient *nc);

/* Id del host al que estamos conectados ahora (para mostrarlo en pantalla). */
int netCurrentHost(NetClient *nc);

/* Cierra la conexion, detiene el hilo receptor y libera recursos. */
void netDisconnect(NetClient *nc);

#endif
