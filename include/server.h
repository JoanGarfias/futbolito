#ifndef SERVER_H
#define SERVER_H

#include "network_packet.h"

/* Servidor del Futbolito. Se usa como programa aparte (futbolito_server, ver
 * server_main.c) o embebido en el cliente, para que un equipo se convierta en
 * host cuando el host anterior se cae (migracion punto a punto).
 *
 * Maneja el roster (que id le toca a cada IP): cada conexion nueva hace un
 * handshake (JoinRequest/JoinResponse) y se le asigna el menor id libre, o
 * el mismo de antes si esta reconectando.
 *
 * serverStart()/serverStartWithRoster() arrancan los hilos (accept + fisica)
 * y regresan de inmediato; serverStop() los detiene. */

int serverStart(void);

/* Para cuando un cliente se convierte en host a medio juego: precarga el
 * roster que ya traia (IPs/ids de los demas). roster == NULL equivale a
 * serverStart(). */
int serverStartWithRoster(const SessionRoster *roster);

void serverStop(void);

#endif
