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
- Explicar qué es el proyecto: *"Un futbolito multijugador en red para hasta 4 jugadores, con código nativo en C compatible con Windows y Linux. Cuenta con una arquitectura Cliente/Servidor basada en sockets TCP, sincronización multihilo por mutex y una característica avanzada de migración de host automática (punto a punto) si el servidor se desconecta."*
- Mostrar el árbol del proyecto (carpetas `src/`, `include/`, `assets/`, `tests/` y archivos `Makefile` y `Makefile.linux`).

### 1:30 – 5:30 · Punto 1 — Multiplataforma con C y APIs nativas (Integrante 2)
- **Explicación del diseño del código:**
  - Abrir `include/threads.h` y explicar que encapsula la API multihilo nativa de ambos SO:
    - Windows → `CreateThread`, `CloseHandle`, `WaitForSingleObject` y `CRITICAL_SECTION`.
    - Linux → `pthread_create`, `pthread_join`, `pthread_detach` y `pthread_mutex_t`.
    - Resaltar que se crearon envolturas comunes (`thread_create`, `mutex_lock`, `thread_sleep_ms`) para evitar llenar la lógica del juego con directivas de compilación `#ifdef`.
  - Abrir `src/server.c` y `src/network.c` para mostrar que las directivas `#ifdef _WIN32` se limitan a la carga de cabeceras de red (`winsock2.h` en Windows frente a `sys/socket.h` y `arpa/inet.h` en Linux) y la definición de macros de sockets (`SOCKET`, `closesocket`, `socket_shutdown`).
  - **No se usan librerías de terceros** para red ni para hilos. Solo **SDL2** para gráficos y entrada.
- **Compilación en vivo:**
  - Windows (MSYS2/MinGW): ejecutar `mingw32-make` y mostrar los ejecutables `build/futbolito.exe` (cliente) y `build/futbolito_server.exe` (servidor dedicado).
  - Linux: ejecutar `make -f Makefile.linux` y mostrar los ejecutables `futbolito` y `futbolito_server`.
- **Prueba rápida:** lanzar el servidor y dos clientes de forma local para demostrar que la ventana abre y responde de inmediato en el SO correspondiente.

### 5:30 – 10:00 · Punto 2 — Arquitectura sockets TCP y Migración de Host (Integrante 3)
- **Sockets y Protocolo de Comunicación:**
  - **Servidor:** abrir `src/server.c` y mostrar el socket pasivo (`socket()`, `bind()`, `listen()`) que escucha conexiones en el puerto 5000 y el hilo receptor (`acceptThread`) que acepta nuevos clientes (`accept()`) y lanza un hilo (`clientThread`) por conexión.
  - **Cliente:** abrir `src/network.c` y mostrar el socket activo que se conecta al servidor (`connect()`).
  - **Paquetes de red:** abrir `include/network_packet.h`. Explicar el handshake de inicio (`JoinRequestPacket` y `JoinResponsePacket`) y los paquetes recurrentes (`PlayerPacket` y `GamePacket`). Explicar cómo el servidor es la fuente de verdad (calcula la física y distribuye el estado) y los clientes envían su input a ~30 fps.
- **Mecanismo de Migración de Host:**
  - Explicar que la red incluye un roster de sesión (`SessionRoster`) que asocia cada jugador activo con su dirección IP. Este roster viaja dentro de cada `GamePacket`.
  - Si la conexión con el servidor se cae (`netIsConnected()` retorna 0), se activa `netBeginReconnect()`, la cual crea un hilo de reconexión (`reconnectWorker`) en segundo plano para no congelar la pantalla.
  - Mostrar la función `computeNextHostAmongActive()` en `src/network.c`. Los clientes calculan de manera determinista quién es el siguiente host activo en el anillo (`1->2->3->4->1`).
  - Si el cliente descubre que *él mismo* es el elegido, inicia su servidor embebido (`serverStartWithRoster()`) heredando el estado del roster. Si es un cliente común, empieza a reintentar la conexión a la IP del nuevo host.
  - Mostrar `renderReconnectOverlay` en `src/render.c` que dibuja el letrero "Reconectando..." mientras el hilo de reconexión realiza su trabajo en segundo plano.

### 10:00 – 14:00 · Punto 3 — Sincronización multihilo y Sección Crítica (Integrante 4)
- **Arquitectura de Hilos concurrentes:**
  - **En el Servidor:** conviven múltiples hilos:
    - 1 hilo para aceptar clientes (`acceptThread`).
    - Hilos dedicados para cada cliente conectado (`clientThread`) leyendo a través del socket.
    - 1 hilo de simulación física (`physicsThread`) actualizando a ~60 fps (16ms).
  - **En el Cliente:** el bucle de renderizado/input de SDL corre en el hilo principal, mientras un hilo receptor (`receiverThread`) lee snapshots de red y el hilo de reconexión (`reconnectWorker`) opera en segundo plano.
- **Sección Crítica (Mutex):**
  - Mostrar cómo el estado global (`GameState`) y el roster (`SessionRoster`) son modificados y leídos por varios hilos a la vez.
  - Mostrar en `src/server.c` cómo las lecturas/escrituras en `physicsThread` y `clientThread` están protegidas dentro de `mutex_lock(&server.mutex)` y `mutex_unlock(&server.mutex)`.
  - Mostrar en `src/network.c` cómo el cliente protege su estado con `netLockState()` / `netUnlockState()`.
- **Demostración de Condición de Carrera:**
  - Explicar qué pasaría sin la sincronización: si el hilo de física lee la pelota al mismo tiempo que un hilo de cliente escribe la interacción de su jugador, se corromperían las variables, habría tirones en las posiciones o caídas del programa por inconsistencia de memoria.
  - Para impresionar (Excelente): comentar rápidamente un mutex y explicar que esto provocaría colisiones inválidas y desincronización inmediata debido al acceso concurrente desprotegido.

### 14:00 – 17:30 · Demo Integral y Prueba de Caída de Servidor (Los 4 integrantes)
- **Escenario 1: Juego Normal:**
  - Conectar 3 o 4 clientes (en la misma red o simulado con 127.0.0.1 en varias terminales).
  - El jugador que crea la sesión corre con `futbolito --host` (Jugador 1 y Host inicial). Los otros corren con `futbolito 127.0.0.1` (o la IP correspondiente).
  - Mostrar en pantalla el partido en curso: los jugadores moviéndose, colisiones con la pelota, goles anotados en el marcador e indicador de ganador.
- **Escenario 2: Caída del Host y Migración en Vivo (Crucial):**
  - **Cerrar abruptamente la ventana del Jugador 1 (Host/Servidor).**
  - Mostrar cómo las ventanas de los jugadores restantes (Jugadores 2 y 3) inmediatamente muestran el overlay "Reconectando..." sin colgarse ni congelar la aplicación.
  - Observar las consolas para ver cómo el Jugador 2 (o el siguiente activo) detecta la caída, inicializa su servidor local y los demás clientes se reconectan automáticamente a él.
  - Mostrar cómo el overlay desaparece y el juego se reanuda con el mismo marcador y estado.

### 17:30 – 18:00 · Conclusiones y Cierre (Integrante 1)
- Resumen de lo aprendido: la importancia de una capa limpia de abstracción multiplataforma, la resolución de condiciones de carrera con secciones críticas y cómo implementar un protocolo de red distribuido y tolerante a fallos mediante sockets.
- Los 4 integrantes a cuadro se despiden.

---

## Checklist antes de publicar
- [ ] Duración entre 15 y 20 min.
- [ ] Aparecen los 4 integrantes a cuadro.
- [ ] Se mostró código Y prueba de los puntos 1, 2 y 3.
- [ ] Se ve corriendo en Linux y en Windows.
- [ ] Se demostró la migración de host (caída en vivo del servidor y reconexión exitosa).
- [ ] Subido a YouTube como **No listado** o **Público**.
- [ ] Copiar el enlace y generar el QR para el cartel.
- [ ] Subir el enlace a MilAulas (los 4 integrantes).
