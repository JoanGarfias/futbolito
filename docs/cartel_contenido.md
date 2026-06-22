# Contenido del cartel (90 × 120 cm)

> Texto listo para el cartel. Revisa/edita los campos entre **[corchetes]**
> (nombres, afiliación, enlace del video). Las tipografías exigidas por la
> rúbrica ya están aplicadas en el archivo PPTX (`docs/cartel_futbolito.pptx`).

---

## TÍTULO (72–100 pt)
**Videojuego "Futbolito" con arquitectura Cliente/Servidor y paralelismo
multihilo sincronizado por sección crítica**

## AUTORES Y AFILIACIONES (36–48 pt)
[Integrante 1], [Integrante 2], [Integrante 3], [Integrante 4]
[Universidad / Facultad] — Sistemas de Cómputo Paralelo y Distribuido — Ciclo 2025-2026B

---

## RESUMEN (texto 24–32 pt)
Se desarrollaron dos aplicaciones nativas —una para **Linux** y otra para
**Windows**— de un videojuego de futbolito multijugador. Ambas comparten una base
en lenguaje **C** y emplean exclusivamente **APIs nativas** de cada sistema
operativo: sockets (Winsock/BSD), hilos (CreateThread/pthread) y mutex
(CRITICAL_SECTION/pthread_mutex). La arquitectura es **Cliente/Servidor** sobre
TCP: un servidor autoritativo calcula la física y distribuye el estado a hasta
4 jugadores. El **paralelismo multihilo** se sincroniza mediante una **sección
crítica con mutex** que protege el estado de juego compartido entre el hilo de
física y los hilos de los clientes, evitando condiciones de carrera.

## INTRODUCCIÓN (24–32 pt)
Los sistemas distribuidos requieren coordinar procesos que se ejecutan en
máquinas distintas y, dentro de cada uno, múltiples hilos que comparten memoria.
Este proyecto materializa ambos conceptos en un videojuego: la comunicación entre
equipos se resuelve con **sockets**, y la concurrencia interna con **hilos** cuya
sincronización se garantiza con el **mecanismo de sección crítica (mutex)**.

## OBJETIVO (24–32 pt)
Desarrollar un par de aplicaciones nativas (Linux y Windows) en C, bajo
arquitectura Cliente/Servidor punto a punto con conectividad heterogénea vía
sockets, y control del paralelismo multihilo mediante sección crítica con mutex,
con una interfaz gráfica que anima las mecánicas del juego de forma distribuida
en hasta 4 equipos.

## DISEÑO Y LÓGICA DE LAS APLICACIONES (24–32 pt + ESQUEMAS)
**Esquema 1 — Arquitectura de red (Cliente/Servidor):**
- Servidor autoritativo: `socket → bind → listen → accept` (un hilo por cliente).
- Clientes: `socket → connect` (conexión TCP persistente). Envían su posición
  (`PlayerPacket`) y reciben el estado global (`GamePacket`).

**Esquema 2 — Modelo multihilo y sección crítica (servidor):**
- Hilo de física (~60 fps) + N hilos de cliente comparten un único `GameState`.
- Cada acceso al estado se envuelve en `mutex_lock() … mutex_unlock()`
  (sección crítica) para evitar condiciones de carrera.

**Esquema 3 — Portabilidad nativa (#ifdef):**
| Recurso | Windows | Linux |
|---|---|---|
| Sockets | Winsock2 (`ws2_32`) | Sockets BSD |
| Hilos | `CreateThread` | `pthread_create` |
| Mutex | `CRITICAL_SECTION` | `pthread_mutex_t` |
| Gráficos | SDL2 | SDL2 |

## RESULTADOS (24–32 pt + IMÁGENES)
- [IMAGEN: captura del juego corriendo en **Windows**]
- [IMAGEN: captura del juego corriendo en **Linux**]
- [IMAGEN: consola del servidor mostrando conexiones de los 4 clientes]
- Hasta 4 jugadores simultáneos; marcador y física consistentes en todas las
  pantallas; sin corrupción de estado gracias al mutex.

## CONCLUSIONES (24–32 pt)
La combinación de sockets para la comunicación distribuida y de hilos
sincronizados por sección crítica permite un videojuego multijugador estable y
portable entre Linux y Windows usando solo C y APIs nativas. El mutex resultó
indispensable: sin él, el acceso concurrente al estado de juego producía
inconsistencias visibles.

## REFERENCIAS (18–22 pt)
1. Stevens, W. R., Fenner, B., Rudoff, A. M. *UNIX Network Programming, Vol. 1*. Addison-Wesley.
2. Microsoft. *Winsock (Windows Sockets 2) Reference*. Microsoft Learn.
3. Microsoft. *Synchronization — Critical Section Objects*. Microsoft Learn.
4. The Open Group. *POSIX Threads (pthreads) Specification*.
5. SDL2. *Simple DirectMedia Layer — API Reference*. libsdl.org.

## QR (enlace al video — punto 4 de la rúbrica)
- [PEGAR QR generado desde: [ENLACE_DEL_VIDEO_YOUTUBE]]
- Etiqueta del QR (20–28 pt): "Video explicativo del proyecto"

---

### Recordatorio de tipografías (rúbrica)
- Título: 72–100 pt · Autores/afiliaciones: 36–48 pt · Subtítulos: 36–44 pt
- Texto general: 24–32 pt · Referencias: 18–22 pt · Etiquetas de figuras: 20–28 pt
- Prioriza **esquemas/figuras** sobre texto.
