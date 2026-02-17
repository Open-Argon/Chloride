/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "thread.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* =========================
   Thread implementation
   ========================= */

int mt_thread_start(mt_thread_t *t, mt_thread_fn fn, void *arg) {
  if (!t || !fn)
    return -1;

#ifdef _WIN32
  t->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fn, arg, 0, NULL);
  if (!t->handle)
    return -1;
#else
  if (pthread_create(&t->thread, NULL, fn, arg) != 0)
    return -1;
#endif

  return 0;
}

int mt_thread_join(mt_thread_t *t) {
#ifdef _WIN32
  WaitForSingleObject(t->handle, INFINITE);
#else
  pthread_join(t->thread, NULL);
#endif
  return 0;
}

int mt_thread_detach(mt_thread_t *t) {
#ifdef _WIN32
  /*
   * Windows threads are detached by closing the handle.
   * The thread keeps running; the OS cleans up on exit.
   */
  if (t->handle) {
    CloseHandle(t->handle);
    t->handle = NULL;
  }
#else
  /*
   * POSIX: mark the thread as detached.
   * After this, pthread_join MUST NOT be called.
   */
  pthread_detach(t->thread);
#endif
  return 0;
}

/* =========================
   Thread ID
   ========================= */

int64_t mt_thread_current_id(void) {
#ifdef _WIN32
    return (int64_t)GetCurrentThreadId();
#else
    return (int64_t)(uintptr_t)pthread_self();
#endif
}

/* =========================
   Mutex
   ========================= */

mt_mutex_t *mt_mutex_create(void) {
  mt_mutex_t *m = calloc(1, sizeof(*m));
  if (!m)
    return NULL;

#ifdef _WIN32
  InitializeCriticalSection(&m->cs);
#else
  pthread_mutex_init(&m->mutex, NULL);
#endif
  return m;
}

void mt_mutex_lock(mt_mutex_t *m) {
#ifdef _WIN32
  EnterCriticalSection(&m->cs);
#else
  pthread_mutex_lock(&m->mutex);
#endif
}

void mt_mutex_unlock(mt_mutex_t *m) {
#ifdef _WIN32
  LeaveCriticalSection(&m->cs);
#else
  pthread_mutex_unlock(&m->mutex);
#endif
}

void mt_mutex_destroy(mt_mutex_t *m) {
#ifdef _WIN32
  DeleteCriticalSection(&m->cs);
#else
  pthread_mutex_destroy(&m->mutex);
#endif
  free(m);
}

/* =========================
   TLS
   ========================= */

mt_tls_t *mt_tls_create(void) {
  mt_tls_t *t = calloc(1, sizeof(*t));
  if (!t)
    return NULL;

#ifdef _WIN32
  t->key = TlsAlloc();
  if (t->key == TLS_OUT_OF_INDEXES) {
    free(t);
    return NULL;
  }
#else
  if (pthread_key_create(&t->key, NULL) != 0) {
    free(t);
    return NULL;
  }
#endif
  return t;
}

void mt_tls_set(mt_tls_t *t, void *value) {
#ifdef _WIN32
  TlsSetValue(t->key, value);
#else
  pthread_setspecific(t->key, value);
#endif
}

void *mt_tls_get(mt_tls_t *t) {
#ifdef _WIN32
  return TlsGetValue(t->key);
#else
  return pthread_getspecific(t->key);
#endif
}

void mt_tls_destroy(mt_tls_t *t) {
#ifdef _WIN32
  TlsFree(t->key);
#else
  pthread_key_delete(t->key);
#endif
  free(t);
}
