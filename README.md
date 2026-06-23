# Futbolito en red 

Proyecto final de **Sistemas de Cómputo Paralelo y Distribuido** (ciclo 2025-2026B).

Es un futbolito multijugador para hasta 4 jugadores. Cada quien controla su
jugador desde su propia computadora y todos ven la misma cancha al mismo tiempo.
Hicimos dos versiones nativas con el mismo código en C: una corre en **Windows**
y otra en **Linux**.

## Integrantes
- [Integrante 1]
- [Integrante 2]
- [Integrante 3]
- [Integrante 4]

## ¿Qué usamos? (lo importante de la materia)
- **Lenguaje C** y APIs nativas de cada sistema operativo (sin librerías raras
  para lo de red ni los hilos):
  - Sockets: **Winsock** en Windows / **sockets BSD** en Linux.
  - Hilos: **CreateThread** en Windows / **pthread** en Linux.
  - Mutex (sección crítica): **CRITICAL_SECTION** en Windows / **pthread_mutex** en Linux.
- **SDL2** (con SDL2_ttf y SDL2_image) solo para dibujar la ventana, el texto
  y los sprites animados de los jugadores.
- Arquitectura **Cliente/Servidor** sobre TCP. El servidor lleva la física
  (la pelota, los goles) y le manda el estado a todos.
- **Migración de host**: si la PC que hace de servidor se cae, otro equipo toma
  su lugar automáticamente (lo "punto a punto").

## Cómo está organizado
```
include/   cabeceras (.h)  -> threads.h tiene el mutex/hilos nativos
src/       código (.c)     -> server.c (servidor), network.c (cliente),
                              physics.c, render.c, input.c, main.c ...
assets/    fuentes y sprites
docs/      guion del video y el cartel
```

## Cómo compilar

### Windows (con MSYS2 / MinGW)
```
mingw32-make
```
Genera `build/futbolito.exe` (cliente) y `build/futbolito_server.exe` (servidor).

> Necesitas SDL2, SDL2_ttf y SDL2_image (nosotros usamos MSYS2 ucrt64):
> `pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_image`
> Si tu SDL está en otra ruta, ajusta `SDLFLAGS` en el `Makefile`.

### Linux
```
make -f Makefile.linux
```
Genera `futbolito` y `futbolito_server`.

> Instala SDL2: `sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev`

## Cómo jugar

**Controles:** WASD para moverte, ESC para salir.

### Importante: no hace falta número de jugador ni lista de IPs
El servidor asigna el id de cada jugador automáticamente (el primero que se
une es el jugador 1, el siguiente el 2, etc.) y recuerda el roster (qué IP
tiene cada id) para la migración de host.

- `futbolito --host` lo corre el primer jugador / quien crea la sesión:
  arranca el servidor embebido y se conecta a sí mismo (queda con el id 1).
- `futbolito <ip_del_host>` lo corren los demás: se conectan a esa IP y el
  servidor les asigna el siguiente id libre (2, 3 o 4).
- `futbolito` (sin argumentos) equivale a `futbolito 127.0.0.1`: solo cliente,
  útil para probar contra un `futbolito_server` dedicado.
- `futbolito_server` sigue existiendo como servidor dedicado fijo para pruebas
  rápidas; no requiere `--host` (ya entiende el mismo protocolo).

Si el host (`--host`) se cae, el siguiente jugador **activo en ese momento**
toma el control automáticamente (arranca su propio servidor embebido) y el
resto se reconecta a él solo, sin congelar la ventana del juego (aparece un
aviso "Reconectando..." mientras tanto). La elección salta a quien no esté
conectado: si el jugador 2 nunca llegó a entrar y se cae el host (jugador 1),
el control pasa directo al jugador 3, no se espera al 2. Si no queda nadie
conectado, la sesión simplemente termina (no se queda reintentando para
siempre).

### Probar en una sola PC (varias ventanas)
```
build/futbolito.exe --host          # 1) crea la sesion (jugador 1, host)
build/futbolito.exe 127.0.0.1       # 2) jugador 2 (otra terminal)
build/futbolito.exe 127.0.0.1       # 3) jugador 3 (otra terminal)
```

### Con varios equipos en red (migración de host automática)
En la PC que crea la sesión:
```
futbolito --host
```
En cada PC adicional, pasándole la IP de la PC anterior:
```
futbolito 192.168.0.11
```
El primer equipo (`--host`) arranca como host con id 1. Si esa PC se apaga, el
jugador 2 toma el control solo (sin que nadie tenga que volver a escribir
nada) y los demás se reconectan a él.

## Sprites
Los sprites de los jugadores van en `assets/sprites/` (ver el README de esa
carpeta). Si no están, el juego dibuja a los jugadores con cuadros de colores.
