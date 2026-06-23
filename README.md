# ⚽ Futbolito en Red (C Nativo)

Proyecto final para la materia de **Sistemas de Cómputo Paralelo y Distribuido**

Es un juego de futbolito multijugador para hasta 4 personas en red local. Cada quien controla a su jugador desde su propia computadora y todos ven la misma cancha en tiempo real. Lo chido de este proyecto es que logramos hacer que jale de forma nativa tanto en **Windows** como en **Linux** usando exactamente el mismo código en C.

## 👥 Integrantes

- Sadie Balboa Betanzos
- Joshua Alexis Horne Gallegos
- Orlando Cabrera Jimenez
- Joan Alvarado Garfias

## 🛠️ ¿Qué tecnologías usamos? (Lo mero bueno de la materia)

Para cumplir con los requisitos del profe y no usar "magia negra" (librerías externas de red o hilos de terceros), programamos todo usando las APIs nativas de cada sistema operativo:

- **Sockets nativos**: Winsock (`winsock2.h`) en Windows y Sockets BSD (`sys/socket.h`) en Linux.
- **Multihilo**: `CreateThread` para Windows y `pthread` en Linux.
- **Sección crítica (Mutex)**: `CRITICAL_SECTION` para Windows y `pthread_mutex_t` en Linux para que los hilos no se pisen al escribir en el estado compartido.
- **SDL2** (con SDL2_ttf y SDL2_image): Esto solo lo usamos para abrir la ventana, cargar la fuente y pintar los sprites de los jugadores (para no tener que dibujar la cancha pixel por pixel a mano).
- **Arquitectura Cliente/Servidor**: Basada en TCP. El servidor es autoritativo (calcula las físicas de la pelota, rebotes y goles) y le manda actualizaciones del estado del juego a todos los clientes.
- **Migración de Host (Punto a Punto)**: Si la compu que está hosteando el servidor se cae o cierra el juego, el siguiente jugador activo toma el control automáticamente en segundo plano sin pausar la partida. ¡Tolerancia a fallos real!

## 📂 Cómo está organizado el proyecto

La estructura está sencillita para no perdernos entre tantos archivos:

```
include/   cabeceras (.h)  -> threads.h hace el paro de unificar los hilos y mutexes nativos de Windows y Linux
src/       código (.c)     -> server.c (servidor), network.c (cliente), physics.c (físicas), render.c (dibujado)...
assets/    fuentes y sprites del juego
docs/      guion del video y el cartel para la presentación
```

## 💻 Cómo compilar y hacerlo jalar

### Windows (usando MSYS2 / MinGW)

Abre la terminal (nosotros usamos la ucrt64 de MSYS2) en la carpeta del proyecto y corre:

```bash
mingw32-make
```

Esto te va a generar dos ejecutables en la carpeta `build/`:

- `build/futbolito.exe` (el juego / cliente)
- `build/futbolito_server.exe` (servidor dedicado para pruebas rápidas)

_Nota:_ Necesitas tener instalado SDL2, SDL2_ttf y SDL2_image. En MSYS2 los instalas rápido con:

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_image
```

Si tus librerías de SDL están en otra ruta, dale una editada al `Makefile` en la variable `SDLFLAGS`.

### Linux

Abre la terminal en la raíz del proyecto y corre:

```bash
make -f Makefile.linux
```

Te va a generar los ejecutables `futbolito` y `futbolito_server` en la raíz del proyecto.

_Nota:_ Para instalar SDL2 en Ubuntu/Debian corre:

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
```

---

## 🎮 Cómo jugar

**Controles:** Te mueves con **WASD** y te sales de la partida con **ESC**.

### Tip: Olvídate de configurar IPs o IDs a mano

El servidor es inteligente: le asigna el ID a cada jugador automáticamente conforme se van conectando (el primero en entrar es el Jugador 1, luego el 2, etc.) y va guardando sus IPs por si se ocupa hacer la migración de host si el primero se desconecta.

- **Para crear una partida (Host):** Corre el juego con el argumento `--host`:

  ```bash
  futbolito --host
  ```

  Esto arranca el servidor embebido en tu compu en segundo plano y te conecta automáticamente como el Jugador 1.

- **Para unirte a una partida:** Pásale la IP del host como argumento:

  ```bash
  futbolito <IP_DEL_HOST>
  ```

  Por ejemplo: `futbolito 192.168.1.75`. El servidor te va a asignar el ID que esté libre (2, 3 o 4).

- **Para jugar contra tu propia compu (Localhost):** Si solo corres `futbolito` sin argumentos, va a intentar conectarse a `127.0.0.1`. Esto sirve si levantaste el `futbolito_server` dedicado en otra terminal.

### ¿Cómo probar la tolerancia a fallos? (Migración de Host)

Si el host original (`--host`) se desconecta o cierra su juego, no pasa nada, la partida no se arruina. El sistema calcula de forma automática quién es el siguiente jugador activo (por ejemplo, el Jugador 2).

Esa compu levanta su propio servidor de respaldo de volada y los demás clientes se reconectan en segundo plano en milisegundos sin congelar la ventana del juego (sale un letrero de "Reconectando..." mientras pasa la magia). Si el Jugador 2 nunca se unió a la partida, se salta directo al Jugador 3. Si ya no queda nadie conectado, la sesión simplemente se cierra.

#### Para probarlo tú solo en tu compu (con varias ventanas):

Abre 3 terminales y corre esto en orden:

```bash
# Terminal 1: Crea la partida (Jugador 1 / Host)
build/futbolito.exe --host

# Terminal 2: Se une como Jugador 2
build/futbolito.exe 127.0.0.1

# Terminal 3: Se une como Jugador 3
build/futbolito.exe 127.0.0.1
```

Si cierras la ventana del Jugador 1, vas a ver cómo en las otras ventanas sale el aviso de "Reconectando..." y de repente el Jugador 2 toma el control del servidor y el juego sigue como si nada con el mismo marcador.

#### Para probarlo con amigos en red local:

En la compu que va a iniciar la partida:

```bash
futbolito --host
```

En las demás compus, le pasan la IP de la primera compu:

```bash
futbolito 192.168.0.11
```

¡Y listo! Si la primera compu se apaga, la segunda se vuelve el host en caliente y los demás se reconectan automáticamente.

## 🎨 Sprites y Gráficos

Los sprites de los monitos y el balón están en `assets/sprites/`. Si por alguna razón el juego no los encuentra o los borras por error, pusimos un sistema de respaldo para que se dibujen como rectángulos de colores según el ID del jugador, ¡así que el juego nunca se va a crashear por falta de imágenes!
