/*
 * Programa servidor independiente (futbolito_server).
 *
 * Solo arranca el modulo servidor y se queda vivo. Util cuando un equipo se
 * dedica a ser el host fijo. Para la version "punto a punto" con migracion de
 * host, el propio cliente embebe este mismo modulo (ver src/server.c y
 * src/network.c).
 */

#include <stdio.h>

#include "../include/server.h"
#include "../include/threads.h"

int main(void)
{
    if (!serverStart())
    {
        printf("No se pudo iniciar el servidor.\n");
        return 1;
    }

    printf("Servidor corriendo. Cierra la ventana para detenerlo.\n");
    fflush(stdout);

    /* El trabajo real lo hacen los hilos; aqui solo mantenemos vivo el main. */
    for (;;)
        thread_sleep_ms(1000);

    return 0;
}
