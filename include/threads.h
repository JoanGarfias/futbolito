#ifndef THREADS_H
#define THREADS_H

/*
 * Capa de abstraccion MULTIHILO usando APIs NATIVAS de cada SO:
 *
 *   - Windows : CRITICAL_SECTION + CreateThread   (windows.h)
 *   - Linux   : pthread_mutex_t  + pthread_create (pthread.h)
 *
 * No se usa ninguna libreria de terceros: cada rama del #ifdef llama
 * directamente a la API nativa del sistema operativo. Asi el mecanismo de
 * SECCION CRITICA con mutex queda implementado de forma nativa en ambos SO.
 */

#ifdef _WIN32

/* En Windows winsock2.h DEBE incluirse antes que windows.h para evitar
 * el conflicto con el viejo winsock.h. Lo garantizamos aqui. */
#include <winsock2.h>
#include <windows.h>

typedef CRITICAL_SECTION Mutex;
typedef HANDLE Thread;

#define THREAD_RET DWORD WINAPI

#else /* Linux / POSIX */

#include <pthread.h>
#include <unistd.h>

typedef pthread_mutex_t Mutex;
typedef pthread_t Thread;

#define THREAD_RET void *

#endif

typedef THREAD_RET (*ThreadFunc)(void *);

/* ----------------------- MUTEX (seccion critica) ----------------------- */

static inline void mutex_init(Mutex *m)
{
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

/* Entra a la SECCION CRITICA: bloquea el mutex. */
static inline void mutex_lock(Mutex *m)
{
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

/* Sale de la SECCION CRITICA: libera el mutex. */
static inline void mutex_unlock(Mutex *m)
{
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}

static inline void mutex_destroy(Mutex *m)
{
#ifdef _WIN32
    DeleteCriticalSection(m);
#else
    pthread_mutex_destroy(m);
#endif
}

/* --------------------------- HILOS (threads) --------------------------- */

/* Crea un hilo nativo. Devuelve 1 en exito, 0 en error. */
static inline int thread_create(Thread *t, ThreadFunc func, void *arg)
{
#ifdef _WIN32
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
    return (*t != NULL);
#else
    return (pthread_create(t, NULL, func, arg) == 0);
#endif
}

static inline void thread_join(Thread t)
{
#ifdef _WIN32
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
#else
    pthread_join(t, NULL);
#endif
}

/* Suelta un hilo que no vamos a esperar con join (ej. el hilo de cada
 * cliente del servidor), para que el SO libere sus recursos al terminar. */
static inline void thread_detach(Thread t)
{
#ifdef _WIN32
    CloseHandle(t);
#else
    pthread_detach(t);
#endif
}

/* Pausa el hilo actual la cantidad de milisegundos indicada. */
static inline void thread_sleep_ms(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

#endif
