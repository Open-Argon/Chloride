/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// The little the custom GC needs from the OS besides memory (see heap.c for
// that): a mutex, a condition variable with a timeout, a monotonic clock.
// Windows uses its native primitives (like RWLock.h does), everything else
// POSIX threads. Used by src/gc/ and runtime/gc_kinds.c.

#ifndef ARGON_GC_PLATFORM_H
#define ARGON_GC_PLATFORM_H

#include <stdbool.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef SRWLOCK gc_mutex;
typedef CONDITION_VARIABLE gc_cond;
#define GC_MUTEX_INIT SRWLOCK_INIT
#define GC_COND_INIT CONDITION_VARIABLE_INIT

static inline void gc_mutex_lock(gc_mutex *m) { AcquireSRWLockExclusive(m); }
static inline void gc_mutex_unlock(gc_mutex *m) { ReleaseSRWLockExclusive(m); }

static inline void gc_cond_wait(gc_cond *c, gc_mutex *m) {
  SleepConditionVariableSRW(c, m, INFINITE, 0);
}

// Returns false if `ms` milliseconds passed without a wake-up. (Like any
// condition variable it can also wake up early; callers re-check.)
static inline bool gc_cond_wait_ms(gc_cond *c, gc_mutex *m, double ms) {
  DWORD wait = ms <= 0 ? 0 : (DWORD)(ms + 1);
  return SleepConditionVariableSRW(c, m, wait, 0) != 0;
}

static inline void gc_cond_broadcast(gc_cond *c) {
  WakeAllConditionVariable(c);
}

static inline double gc_now_ms(void) {
  static LARGE_INTEGER frequency;
  LARGE_INTEGER now;
  if (!frequency.QuadPart)
    QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&now);
  return (double)now.QuadPart * 1000.0 / (double)frequency.QuadPart;
}

#else // POSIX: Linux, macOS, the BSDs

#include <pthread.h>
#include <time.h>

typedef pthread_mutex_t gc_mutex;
typedef pthread_cond_t gc_cond;
#define GC_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#define GC_COND_INIT PTHREAD_COND_INITIALIZER

static inline void gc_mutex_lock(gc_mutex *m) { pthread_mutex_lock(m); }
static inline void gc_mutex_unlock(gc_mutex *m) { pthread_mutex_unlock(m); }

static inline void gc_cond_wait(gc_cond *c, gc_mutex *m) {
  pthread_cond_wait(c, m);
}

// Returns false if `ms` milliseconds passed without a wake-up. (Like any
// condition variable it can also wake up early; callers re-check.)
static inline bool gc_cond_wait_ms(gc_cond *c, gc_mutex *m, double ms) {
  struct timespec deadline; // pthread_cond_timedwait wants CLOCK_REALTIME
  clock_gettime(CLOCK_REALTIME, &deadline);
  long long ns = deadline.tv_nsec + (long long)(ms * 1e6);
  deadline.tv_sec += (time_t)(ns / 1000000000LL);
  deadline.tv_nsec = (long)(ns % 1000000000LL);
  return pthread_cond_timedwait(c, m, &deadline) == 0;
}

static inline void gc_cond_broadcast(gc_cond *c) { pthread_cond_broadcast(c); }

static inline double gc_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

#endif

#endif // ARGON_GC_PLATFORM_H
