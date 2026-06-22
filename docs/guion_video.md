# Guion del video — Futbolito Cliente/Servidor (15–20 min)

**Requisitos de la rúbrica (punto 4):**
- Duración: mínimo 15, máximo 20 minutos.
- Deben **salir a cuadro los 4 integrantes**.
- Cubrir en **código y prueba** los 3 primeros puntos: (1) funcionamiento en Linux y
  Windows con C y APIs nativas, (2) arquitectura Cliente/Servidor con sockets,
  (3) sincronización multihilo con mutex (sección crítica).
- Publicar en YouTube como **No listado o Público**.

> Sugerencia: graben pantalla con OBS y una webcam pequeña en una esquina para que
> siempre haya alguien "a cuadro". Hagan una pasada de cada integrante hablando.

---

## Estructura por bloques (objetivo: ~18 min)

### 0:00 – 1:30 · Introducción (Integrante 1)
- Los 4 integrantes a cuadro, se presentan con nombre y rol.
- Explicar en una frase qué es el proyecto: *"Un futbolito multijugador en red,
  con una app nativa para Windows y otra para Linux, arquitectura
  Cliente/Servidor sobre sockets y sincronización multihilo con mutex."*
- Mostrar el árbol del proyecto (carpeta `src/`, `include/`, `Makefile`,
  `Makefile.linux`).

### 1:30 – 5:30 · Punto 1 — Funciona en Linux y Windows con C y APIs nativas (Integrante 2)
- **Código:** abrir `src/server.c` y `src/network.c`. Señalar que TODO es C y que
  cada llamada a SO está dentro de `#ifdef _WIN32 ... #else ...`:
  - Windows → `winsock2.h`, `WSAStartup`, `CreateThread`, `CRITICAL_SECTION`.
  - Linux → `sys/socket.h`, `pthread_create`, `pthread_mutex_t`.
  - Mostrar `include/threads.h`: una sola cabecera con las dos implementaciones
    nativas. Recalcar: **sin librerías de terceros para red ni hilos**.
- **Prueba:** mostrar la compilación en ambos SO:
  - Windows: `mingw32-make` → genera `build/futbolito.exe` y `build/futbolito_server.exe`.
  - Linux: `make -f Makefile.linux` → genera `futbolito` y `futbolito_server`.
- Arrancar el servidor y 2–4 clientes; mover los jugadores y meter un gol.
  Mostrar que funciona en Windows **y** en Linux (idealmente una máquina de cada uno).

### 5:30 – 9:30 · Punto 2 — Arquitectura Cliente/Servidor con sockets (Integrante 3)
- **Diagrama:** mostrar el esquema (el del cartel sirve): 1 servidor autoritativo
  + 4 clientes; el servidor calcula la física y reparte el estado.
- **Código del servidor** (`src/server.c`):
  - `socket()` → `bind()` → `listen()` → `accept()` en bucle.
  - Por cada `accept()` se lanza un hilo de cliente (conexión TCP persistente).
- **Código del cliente** (`src/network.c`):
  - `netConnect()`: `socket()` → `connect()` una sola vez (conexión persistente).
  - El cliente envía `PlayerPacket` (su posición) y recibe `GamePacket` (estado global).
  - Mostrar `include/network_packet.h` (los paquetes que viajan por la red).
- **Prueba:** abrir el servidor y mostrar en su consola los mensajes
  `[SERVIDOR] Nueva conexion desde <IP>` cuando entran los clientes.

### 9:30 – 14:30 · Punto 3 — Multihilo + mutex (sección crítica) (Integrante 4)
- **Concepto:** el servidor tiene varios hilos que tocan el MISMO `GameState`:
  - 1 hilo de física (`physicsThread`) a ~60 fps.
  - 1 hilo por cada cliente (`clientThread`).
  - El cliente además tiene su `receiverThread`.
- **Código:** en `src/server.c` mostrar el `GameState game` compartido y cómo cada
  acceso está envuelto entre `mutex_lock(&server.mutex)` y
  `mutex_unlock(&server.mutex)` — **esa es la sección crítica**.
- Abrir `include/threads.h` y mostrar que `mutex_lock` es
  `EnterCriticalSection` en Windows y `pthread_mutex_lock` en Linux.
- **Demostración de la condición de carrera (clave para "Excelente"):**
  - Explicar: si se quitara el mutex, el hilo de física y los hilos de clientes
    escribirían la pelota y las posiciones al mismo tiempo → estado corrupto,
    tirones, valores inconsistentes.
  - Opcional para impresionar: tener una rama/comentario donde se quita el lock
    y mostrar el glitch; luego con el lock todo es estable.
- **Prueba:** 3–4 clientes moviéndose y chutando a la vez; el juego se mantiene
  consistente en todas las pantallas.

### 14:30 – 17:30 · Demo integral distribuida (los 4 integrantes)
- Correr el sistema completo: 1 servidor + 4 clientes (idealmente en 4 equipos;
  si no, varias ventanas con `127.0.0.1`).
- Partido real: goles, marcador actualizándose en todas las pantallas.

### 17:30 – 18:00 · Cierre (Integrante 1)
- Conclusiones: qué aprendieron de sockets, hilos y secciones críticas.
- Los 4 a cuadro despidiéndose.

---

## Checklist antes de publicar
- [ ] Duración entre 15 y 20 min.
- [ ] Aparecen los 4 integrantes a cuadro.
- [ ] Se mostró código Y prueba de los puntos 1, 2 y 3.
- [ ] Se ve corriendo en Linux y en Windows.
- [ ] Subido a YouTube como **No listado** o **Público**.
- [ ] Copiar el enlace y generar el QR para el cartel.
- [ ] Subir el enlace a MilAulas (los 4 integrantes).
