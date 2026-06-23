/* futbolito_server: arranca el servidor y se queda vivo, para cuando un
 * equipo quiere ser host fijo. En modo punto a punto el cliente ya trae
 * este mismo modulo embebido (ver server.c / network.c). */

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
