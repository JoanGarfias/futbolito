#ifndef SERVER_H
#define SERVER_H

#include "network_packet.h"

/*
 * Modulo del servidor del Futbolito.
 *
 * Se puede usar de dos formas:
 *   - Como programa aparte (futbolito_server) -> ver src/server_main.c
 *   - Embebido dentro del cliente, para que un equipo pueda CONVERTIRSE en
 *     host cuando el host anterior se cae (migracion de host / punto a punto).
 *
 * El servidor es la fuente de verdad del roster (que id le toca a cada IP):
 * cada conexion nueva hace un handshake (JoinRequest/JoinResponse, ver
 * network_packet.h) y el servidor le asigna el menor id libre (o le devuelve
 * el mismo id de antes si reconecta). SOLO el equipo elegido como host arranca
 * el servidor embebido; los demas unicamente se conectan a el.
 *
 * serverStart() / serverStartWithRoster() arrancan los hilos (accept + fisica)
 * en segundo plano y regresan de inmediato. serverStop() los detiene.
 */

/* Arranca el servidor con una sesion nueva (roster vacio). Equivalente a
 * serverStartWithRoster(NULL). Devuelve 1 si todo bien, 0 si fallo (por
 * ejemplo si el puerto ya esta ocupado). */
int serverStart(void);

/* Arranca el servidor precargando el roster de una sesion existente (caso de
 * migracion: el nuevo host ya conoce las IPs/ids de los demas equipos). Si
 * roster es NULL, equivale a serverStart() (sesion nueva). */
int serverStartWithRoster(const SessionRoster *roster);

/* Detiene el servidor y libera sus recursos. */
void serverStop(void);

#endif
