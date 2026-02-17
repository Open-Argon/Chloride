/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef MINI_THREAD_H
#define MINI_THREAD_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================
   Internal structs
   ========================= */

struct mt_thread {
#ifdef _WIN32
  HANDLE handle;
#else
  pthread_t thread;
#endif
};

struct mt_mutex {
#ifdef _WIN32
  CRITICAL_SECTION cs;
#else
  pthread_mutex_t mutex;
#endif
};

struct mt_tls {
#ifdef _WIN32
  DWORD key;
#else
  pthread_key_t key;
#endif
};

/* =========================
   Thread types
   ========================= */

typedef struct mt_thread mt_thread_t;
typedef struct mt_mutex mt_mutex_t;

/* Opaque thread ID */
typedef struct {
  unsigned char bytes[32];
  size_t size;
} mt_thread_id_t;

/* Thread entry function */
typedef void *(*mt_thread_fn)(void *arg);

/* =========================
   Thread API
   ========================= */

int mt_thread_start(mt_thread_t *t, mt_thread_fn fn, void *arg);
int mt_thread_join(mt_thread_t *t);
int mt_thread_detach(mt_thread_t *t);

int64_t mt_thread_current_id(void);

/* =========================
   Mutex API
   ========================= */

mt_mutex_t *mt_mutex_create(void);
void mt_mutex_lock(mt_mutex_t *m);
void mt_mutex_unlock(mt_mutex_t *m);
void mt_mutex_destroy(mt_mutex_t *m);

/* =========================
   TLS (thread-local storage)
   ========================= */

typedef struct mt_tls mt_tls_t;

mt_tls_t *mt_tls_create(void);
void mt_tls_set(mt_tls_t *tls, void *value);
void *mt_tls_get(mt_tls_t *tls);
void mt_tls_destroy(mt_tls_t *tls);

#ifdef __cplusplus
}
#endif

#endif
