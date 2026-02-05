/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hashmap.h"
#include "../../../memory.h"

#include <string.h>
#include <stdlib.h>

/* ===========================
   Internal helpers (NO LOCKS)
   =========================== */

static inline int hashCode_GC_nolock(struct hashmap_GC *t, uint64_t hash) {
  return t->size ? (hash & (t->size - 1)) : 0;
}

static void hashmap_insert_GC_nolock(
    struct hashmap_GC *t,
    uint64_t hash,
    void *key,
    void *val,
    size_t order
);

static void resize_hashmap_GC_nolock(struct hashmap_GC *t) {
  int old_size = t->size;
  int new_size = old_size ? old_size * 2 : 8;

  struct node_GC **old_list = t->list;

  t->list = ar_alloc(sizeof(struct node_GC *) * new_size);
  memset(t->list, 0, sizeof(struct node_GC *) * new_size);
  t->size = new_size;
  t->hashmap_count = 0;

  if (!old_list)
    return;

  for (int i = 0; i < old_size; i++) {
    struct node_GC *temp = old_list[i];
    while (temp) {
      hashmap_insert_GC_nolock(
        t,
        temp->hash,
        temp->key,
        temp->val,
        temp->order
      );
      temp = temp->next;
    }
  }
}

static void hashmap_insert_GC_nolock(
    struct hashmap_GC *t,
    uint64_t hash,
    void *key,
    void *val,
    size_t order
) {
  if (!order)
    order = t->order++;

  /* Inline storage */
  for (size_t i = 0; i < t->inline_count; i++) {
    if (t->inline_values[i].hash == hash) {
      t->inline_values[i].val = val;
      return;
    }
  }

  if (!t->list && t->inline_count < INLINE_HASHMAP_ARRAY_SIZE) {
    t->inline_values[t->inline_count++] = (struct node_GC){
      .hash = hash,
      .key = key,
      .val = val,
      .order = order,
      .next = NULL
    };
    t->count++;
    return;
  }

  if ((t->hashmap_count + 1) > t->size * 0.75) {
    resize_hashmap_GC_nolock(t);
  }

  int pos = hashCode_GC_nolock(t, hash);
  struct node_GC *temp = t->list[pos];

  while (temp) {
    if (temp->hash == hash) {
      temp->val = val;
      return;
    }
    temp = temp->next;
  }

  struct node_GC *n = ar_alloc(sizeof(struct node_GC));
  *n = (struct node_GC){
    .hash = hash,
    .key = key,
    .val = val,
    .order = order,
    .next = t->list[pos]
  };

  t->list[pos] = n;
  t->hashmap_count++;
  t->count++;
}

/* ===========================
   Public API (LOCKED)
   =========================== */

struct hashmap_GC *createHashmap_GC(void) {
  struct hashmap_GC *t = ar_alloc(sizeof(struct hashmap_GC));
  memset(t, 0, sizeof(*t));
  t->order = 1;
  pthread_rwlock_init(&t->lock, NULL);
  return t;
}

void clear_hashmap_GC(struct hashmap_GC *t) {
  pthread_rwlock_wrlock(&t->lock);

  t->order = 1;
  t->count = 0;
  t->inline_count = 0;
  t->hashmap_count = 0;

  if (t->list)
    memset(t->list, 0, sizeof(struct node_GC *) * t->size);

  pthread_rwlock_unlock(&t->lock);
}

struct node_GC **hashmap_GC_to_array(
    struct hashmap_GC *t,
    size_t *array_length
) {
  pthread_rwlock_rdlock(&t->lock);

  size_t cap = 8;
  *array_length = 0;
  struct node_GC **arr = ar_alloc(cap * sizeof(*arr));

  for (size_t i = 0; i < t->inline_count; i++) {
    if (*array_length == cap) {
      cap *= 2;
      arr = ar_realloc(arr, cap * sizeof(*arr));
    }
    arr[(*array_length)++] = &t->inline_values[i];
  }

  for (size_t i = 0; i < t->size; i++) {
    struct node_GC *n = t->list[i];
    while (n) {
      if (*array_length == cap) {
        cap *= 2;
        arr = ar_realloc(arr, cap * sizeof(*arr));
      }
      arr[(*array_length)++] = n;
      n = n->next;
    }
  }

  pthread_rwlock_unlock(&t->lock);
  return arr;
}

int hashmap_remove_GC(struct hashmap_GC *t, uint64_t hash) {
  pthread_rwlock_wrlock(&t->lock);

  for (size_t i = 0; i < t->inline_count; i++) {
    if (t->inline_values[i].hash == hash) {
      memmove(
        &t->inline_values[i],
        &t->inline_values[i + 1],
        (t->inline_count - i - 1) * sizeof(struct node_GC)
      );
      t->inline_count--;
      pthread_rwlock_unlock(&t->lock);
      return 1;
    }
  }

  if (!t->list) {
    pthread_rwlock_unlock(&t->lock);
    return 0;
  }

  int pos = hashCode_GC_nolock(t, hash);
  struct node_GC *prev = NULL;
  struct node_GC *cur = t->list[pos];

  while (cur) {
    if (cur->hash == hash) {
      if (prev)
        prev->next = cur->next;
      else
        t->list[pos] = cur->next;

      pthread_rwlock_unlock(&t->lock);
      return 1;
    }
    prev = cur;
    cur = cur->next;
  }

  pthread_rwlock_unlock(&t->lock);
  return 0;
}

void hashmap_insert_GC(
    struct hashmap_GC *t,
    uint64_t hash,
    void *key,
    void *val,
    size_t order
) {
  pthread_rwlock_wrlock(&t->lock);
  hashmap_insert_GC_nolock(t, hash, key, val, order);
  pthread_rwlock_unlock(&t->lock);
}

void *hashmap_lookup_GC(struct hashmap_GC *t, uint64_t hash) {
  pthread_rwlock_rdlock(&t->lock);

  for (size_t i = 0; i < t->inline_count; i++) {
    if (t->inline_values[i].hash == hash) {
      void *v = t->inline_values[i].val;
      pthread_rwlock_unlock(&t->lock);
      return v;
    }
  }

  if (!t->list) {
    pthread_rwlock_unlock(&t->lock);
    return NULL;
  }

  int pos = hashCode_GC_nolock(t, hash);
  struct node_GC *n = t->list[pos];

  while (n) {
    if (n->hash == hash) {
      void *v = n->val;
      pthread_rwlock_unlock(&t->lock);
      return v;
    }
    n = n->next;
  }

  pthread_rwlock_unlock(&t->lock);
  return NULL;
}
