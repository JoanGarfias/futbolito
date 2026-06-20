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
- **SDL2** solo para dibujar la ventana y los jugadores.
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

> Necesitas SDL2 y SDL2_ttf instalados (nosotros usamos MSYS2 ucrt64). Si tu
> SDL está en otra ruta, ajusta `SDLFLAGS` en el `Makefile`.

### Linux
```
make -f Makefile.linux
```
Genera `futbolito` y `futbolito_server`.

> Instala SDL2: `sudo apt install libsdl2-dev libsdl2-ttf-dev`

## Cómo jugar

**Controles:** WASD para moverte, ESC para salir.

### Probar en una sola PC (varias ventanas)
```
build/futbolito_server.exe          # 1) el servidor
build/futbolito.exe 1               # 2) jugador 1
build/futbolito.exe 2               # 3) jugador 2 (otra terminal)
```

### Con 4 equipos en red (modo punto a punto con migración de host)
En cada computadora se corre el cliente pasándole su número de jugador y la
lista de las 4 IPs (en el mismo orden en todas las máquinas):
```
futbolito 1 192.168.0.11 192.168.0.12 192.168.0.13 192.168.0.14
futbolito 2 192.168.0.11 192.168.0.12 192.168.0.13 192.168.0.14
...
```
El equipo 1 arranca como host. Si esa PC se apaga, el equipo 2 toma el control
solo y los demás se reconectan a él.

## Sprites
Los sprites de los jugadores van en `assets/sprites/` (ver el README de esa
carpeta). Si no están, el juego dibuja a los jugadores con cuadros de colores.
