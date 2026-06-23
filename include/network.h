#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"

/* Cliente de red, con migracion de host automatica si el servidor se cae.
 *
 * NetClient es opaco (el struct real vive en network.c) para que este header
 * no arrastre winsock2.h/windows.h y choque con SDL en main.c.
 *
 * El id de jugador y las IPs ya NO se pasan por consola: el servidor asigna
 * el id por handshake (ver network_packet.h) y manda el roster (que IP tiene
 * cada id) en cada snapshot. netConnect(ip, isSessionHost) crea la sesion
 * (isSessionHost=1, arranca el servidor embebido y te quedas con id 1) o se
 * conecta a una existente (isSessionHost=0, te asignan el primer id libre).
 *
 * Si el host se cae, se elige como nuevo host al siguiente id ACTIVO en el
 * anillo 1->2->3->4->1 (no nada mas host+1, porque puede haber ids que nunca
 * se conectaron). Esa migracion corre en un hilo aparte
 * (netBeginReconnect/netPollReconnect) para no trabar el bucle de SDL.
 *
 * Varios hilos tocan el mismo GameState/roster, asi que todo acceso va
 * dentro de netLockState()/netUnlockState(), o usando los getters de aqui
 * abajo (ya hacen el lock por dentro). */

typedef struct NetClient NetClient;

typedef enum
{
    NET_STATE_CONNECTED,
    NET_STATE_RECONNECTING,
    NET_STATE_DISCONNECTED
} NetState;

/* isSessionHost=1 crea la sesion (servidor embebido, id 1); con 0 se conecta
 * a hostIp y recibe el id que le toque. Devuelve NULL si no logro conectar. */
NetClient *netConnect(const char *hostIp, int isSessionHost, GameState *game);

int netGetMyId(NetClient *nc);
NetState netGetState(NetClient *nc);

void netSendLocalPlayer(NetClient *nc, GameState *game);

void netLockState(NetClient *nc);
void netUnlockState(NetClient *nc);

int netIsConnected(NetClient *nc);

/* Arranca la migracion en segundo plano (no bloquea). Llamar al detectar
 * netIsConnected() == 0. */
void netBeginReconnect(NetClient *nc);

/* Llamar cada frame mientras siga en RECONNECTING (para pintar el overlay).
 * Devuelve 1 cuando ya quedo CONNECTED otra vez. */
int netPollReconnect(NetClient *nc);

int netCurrentHost(NetClient *nc);

void netDisconnect(NetClient *nc);

#endif
