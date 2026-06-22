#ifndef SERVER_H
#define SERVER_H

/*
 * Modulo del servidor del Futbolito.
 *
 * Se puede usar de dos formas:
 *   - Como programa aparte (futbolito_server) -> ver src/server_main.c
 *   - Embebido dentro del cliente, para que un equipo pueda CONVERTIRSE en
 *     host cuando el host anterior se cae (migracion de host / punto a punto).
 *
 * serverStart() arranca los hilos (accept + fisica) en segundo plano y regresa
 * de inmediato. serverStop() los detiene.
 */

/* Arranca el servidor en segundo plano. Devuelve 1 si todo bien, 0 si fallo
 * (por ejemplo si el puerto ya esta ocupado). */
int serverStart(void);

/* Detiene el servidor y libera sus recursos. */
void serverStop(void);

#endif
