#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"

/*
 * CLIENTE de red del Futbolito (con migracion de host automatica).
 *
 * NetClient es un tipo OPACO: su definicion real (socket, mutex, hilos) vive
 * en network.c. Asi este header NO arrastra winsock2.h/windows.h, evitando
 * conflictos con SDL en main.c.
 *
 * No hace falta pasar el id de jugador ni la lista de IPs por linea de
 * comandos: el SERVIDOR es la fuente de verdad del roster (que IP tiene cada
 * id) y lo asigna/recuerda via handshake (ver network_packet.h). Cada cliente
 * guarda una copia local de ese roster para poder decidir el siguiente host
 * si la conexion se cae.
 *
 *   - netConnect(ip, isSessionHost=1, ...)  -> crea la sesion: arranca el
 *     servidor embebido y se conecta a si mismo (queda con id 1).
 *   - netConnect(ip, isSessionHost=0, ...)  -> se conecta a esa IP; el
 *     servidor le asigna el primer id libre (2, 3, 4...).
 *
 * Si el host se cae, SOLO el equipo elegido arranca el servidor embebido; el
 * resto unicamente reintenta connect() contra el. El elegido es el siguiente
 * id ACTIVO (conectado ahora mismo, no solo alguna vez registrado) en el
 * anillo 1->2->3->4->1 a partir del host caido -- NO simplemente "host+1": si
 * el jugador 2 nunca entro y cae el host 1, el host 1 se salta y el elegido
 * es el jugador 3. Si no queda ningun jugador activo, la sesion termina
 * (no hay bucle infinito de reconexion). La migracion corre en un hilo en
 * segundo plano (netBeginReconnect/netPollReconnect) para no congelar el
 * bucle SDL.
 *
 * El hilo principal (SDL), el hilo receptor y el hilo de reconexion acceden
 * al mismo GameState/roster, por eso TODO acceso debe ir entre
 * netLockState()/netUnlockState() (seccion critica) o usar los getters
 * provistos (que ya hacen el lock internamente).
 */

typedef struct NetClient NetClient;

typedef enum
{
    NET_STATE_CONNECTED,
    NET_STATE_RECONNECTING,
    NET_STATE_DISCONNECTED
} NetState;

/* Conecta al juego. hostIp es la IP a la que conectarse (o desde la que se
 * sirve, si isSessionHost). isSessionHost=1 arranca el servidor embebido y
 * crea la sesion (recibira el id 1); isSessionHost=0 solo se conecta como
 * cliente y recibe el primer id libre que le asigne el servidor. Bloquea
 * brevemente con reintentos acotados al inicio; devuelve NULL si no logra
 * conectar. */
NetClient *netConnect(const char *hostIp, int isSessionHost, GameState *game);

/* Id de jugador asignado por el servidor en el handshake (0 si todavia no se
 * conecto ninguna vez, lo cual no deberia pasar tras un netConnect exitoso). */
int netGetMyId(NetClient *nc);

/* Estado actual de la conexion: CONNECTED, RECONNECTING (migracion en curso
 * en segundo plano) o DISCONNECTED (no hay host disponible). */
NetState netGetState(NetClient *nc);

/* Envia la posicion del jugador local al host actual. No hace nada si el
 * estado no es NET_STATE_CONNECTED (p.ej. durante una reconexion). */
void netSendLocalPlayer(NetClient *nc, GameState *game);

/* Entra/sale de la seccion critica que protege el GameState compartido. */
void netLockState(NetClient *nc);
void netUnlockState(NetClient *nc);

/* 1 si la conexion sigue viva, 0 si se perdio. */
int netIsConnected(NetClient *nc);

/* Lanza la migracion de host en un hilo en segundo plano (no bloquea el
 * llamador). Llamar una vez, desde el bucle principal, al notar que
 * netIsConnected() devuelve 0. Si ya hay una reconexion en curso, no hace
 * nada. */
void netBeginReconnect(NetClient *nc);

/* Punto de sondeo no bloqueante: llamar cada frame mientras
 * netGetState() == NET_STATE_RECONNECTING (p.ej. para dibujar un overlay).
 * Devuelve 1 si ya esta NET_STATE_CONNECTED de nuevo. */
int netPollReconnect(NetClient *nc);

/* Id del host actual segun el roster sincronizado (para mostrarlo en pantalla). */
int netCurrentHost(NetClient *nc);

/* Cierra la conexion, detiene los hilos y libera recursos. */
void netDisconnect(NetClient *nc);

#endif
