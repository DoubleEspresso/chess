#pragma once

#ifndef SBC_PLATFORM_H
#define SBC_PLATFORM_H

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// mac-osx
#ifdef __APPLE__
#ifdef THREADED
#include <pthread.h>   
typedef pthread_mutex_t MUTEX;
typedef pthread_t THREAD;
typedef pthread_cond_t CONDITION;
typedef void*(*thread_fnc)(void*);

#define thread_lock(x)   pthread_mutex_lock(&(x))
#define thread_unlock(x) pthread_mutex_unlock(&(x))
#define thread_create(x, f, t) pthread_create(&(x), NULL, (thread_fnc)f, t)
#define thread_join(x)  pthread_join(x,NULL)
#define thread_signal(x) pthread_cond_signal(&(x))
#define thread_broadcast(x)  pthread_cond_broadcast(&(x))
#define thread_wait(x) pthread_cond_wait(&(x))
#define thread_sleep(x) sleep(x)
#endif
#elif __linux
// linux
#ifdef THREADED
extern "C" {
#include <pthread.h>
#include <unistd.h>
}
typedef pthread_mutex_t MUTEX;
typedef pthread_t THREAD;
typedef pthread_cond_t CONDITION;
typedef void*(*thread_fnc)(void*);

#define mutex_init(x)   pthread_mutex_init(&(x), NULL);
#define mutex_lock(x)   pthread_mutex_lock(&(x))
#define mutex_unlock(x) pthread_mutex_unlock(&(x))

#define thread_create(x, f, t) pthread_create(&(x), NULL, (thread_fnc)f, t)
#define thread_join(x)  pthread_join(x,NULL)
#define thread_signal(x) pthread_cond_signal(&(x))
#define thread_cond_init(x) pthread_cond_init(&(x),NULL)
#define thread_broadcast(x)  pthread_cond_broadcast(&(x))
#define thread_wait(x,y) pthread_cond_wait(&(x),&(y))
#define thread_timed_wait(x,y,z) pthread_cond_timedwait(&(x),&(y),&(z))
#define thread_sleep(x) usleep(x)
#define cond_destroy(x) pthread_cond_destroy(&(x))
#endif
//#ifdef _64BIT
#  define builtin_popcount __builtin_popcountll
#  define builtin_lsb __builtin_ffsll
//#else 
//#  define builtin_popcount __builtin_popcount
//#  define builtin_lsb __builtin_ffs
//#endif
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#elif _MSC_VER
// windows
#ifdef THREADED
#include <windows.h>   
typedef CRITICAL_SECTION MUTEX;
typedef HANDLE CONDITION;
typedef DWORD THREAD;

typedef void*(*thread_fnc)(void*);

// wrappers for win32 threads
namespace 
{
	// create an auto-reset event.
	void create_event(HANDLE& h) { h = CreateEvent(NULL, FALSE, FALSE, NULL); }
	void cond_wait (HANDLE * h, CRITICAL_SECTION * external_mutex)
	{
		// Release the <external_mutex> here and wait for either event
		// to become signaled, due to <pthread_cond_signal> being
		// called or <pthread_cond_broadcast> being called.
		LeaveCriticalSection (external_mutex);
		WaitForMultipleObjects (1, h, FALSE, INFINITE); 

		// Reacquire the mutex before returning.
		EnterCriticalSection (external_mutex);
	}
	void timed_wait(HANDLE *h, CRITICAL_SECTION * external_mutex, int time_ms)
	{
		LeaveCriticalSection(external_mutex);
		WaitForMultipleObjects (1, h, FALSE, (DWORD) time_ms); 
		EnterCriticalSection (external_mutex);
	}
}

#define mutex_lock(x)   EnterCriticalSection(&(x))
#define mutex_unlock(x) LeaveCriticalSection(&(x))

#define thread_cond_init(x) create_event(x);
#define thread_wait(x,y) cond_wait (&(x),&(y))
#define thread_timed_wait(x,y,z) timed_wait(&(x),&(y),z)
#define thread_signal(x) SetEvent(x)
#define cond_destroy(x) CloseHandle(x)
#define thread_create(x, f, t) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, t, 0, &(x))
#define thread_sleep(x) Sleep(x)
//#define thread_join(x)  pthread_join(x,NULL)

// #define thread_broadcast(x)  pthread_cond_broadcast(&(x))
// #define thread_wait(x) pthread_cond_wait(&(x))
#endif
#ifdef _64BIT
#include <smmintrin.h>
#include <intrin.h>
#  define builtin_popcount _mm_popcnt_u64 
#  define builtin_lsb _BitScanForward64
#else 
#  include <intrin.h>
#  define builtin_popcount __popcnt
#  define builtin_lsb _BitScanForward
#endif
#endif

#endif
