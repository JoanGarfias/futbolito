#ifndef THREADS_H
#define THREADS_H

/* Wrapper de hilos/mutex para no llenar el resto del codigo de #ifdef.
 * Windows usa CreateThread + CRITICAL_SECTION, Linux usa pthreads. Sin
 * librerias externas, solo lo nativo de cada SO. */

#ifdef _WIN32

/* ojo: winsock2.h tiene que ir ANTES que windows.h, si no truena con el
 * winsock viejo que windows.h mete por su cuenta */
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

static inline void mutex_init(Mutex *m)
{
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

static inline void mutex_lock(Mutex *m)
{
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

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

/* 1 si pudo crear el hilo, 0 si fallo */
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

/* para hilos que no vamos a esperar con join (los de cada cliente del
 * servidor), asi el SO libera los recursos solo al terminar */
static inline void thread_detach(Thread t)
{
#ifdef _WIN32
    CloseHandle(t);
#else
    pthread_detach(t);
#endif
}

static inline void thread_sleep_ms(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

#endif
