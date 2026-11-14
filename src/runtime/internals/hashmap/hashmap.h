/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HASHMAP_GC_H
#define HASHMAP_GC_H

#include <stddef.h>
#include <stdint.h>
#include "../../../RWLock.h"
#include "../../../gc/gc.h"

#define INLINE_HASHMAP_ARRAY_SIZE 3

struct node_GC {
  uint64_t hash;
  void *key;
  void *val;
  size_t order;
  struct node_GC *next;
};

struct hashmap_GC {
  size_t size;
  size_t order;
  struct node_GC **list;

  size_t hashmap_count;
  size_t count;
  size_t inline_count;
  struct node_GC inline_values[INLINE_HASHMAP_ARRAY_SIZE];

  RWLock lock; // switched to read-write lock

  // Which of keys / values are pointers to managed objects. The GC traverses
  // only those; maps with other keys or values (C strings, flags...) must say
  // so when they're created.
  uint8_t gc_flags;
};

#define HASHMAP_GC_OBJECT_KEYS 1
#define HASHMAP_GC_OBJECT_VALUES 2
#define HASHMAP_GC_OBJECTS (HASHMAP_GC_OBJECT_KEYS | HASHMAP_GC_OBJECT_VALUES)

/* Public API (thread-safe) */

// Keys and values are both managed objects (or NULL): the usual case.
struct hashmap_GC *createHashmap_GC(void);
// Same, but say which of keys/values are managed objects (HASHMAP_GC_*).
struct hashmap_GC *createHashmap_GC_with(uint8_t gc_flags);

// Visit every key and value that is a managed object (see gc_flags).
void hashmap_GC_traverse(struct hashmap_GC *t, gc_visit_fn visit, void *ctx);

void clear_hashmap_GC(struct hashmap_GC *t);

struct node_GC **hashmap_GC_to_array(
    struct hashmap_GC *t,
    size_t *array_length
);

int hashmap_remove_GC(struct hashmap_GC *t, uint64_t hash);

void hashmap_insert_GC(
    struct hashmap_GC *t,
    uint64_t hash,
    void *key,
    void *val,
    size_t order
);

void *hashmap_lookup_node_GC(struct hashmap_GC *t, uint64_t hash);

void *hashmap_lookup_GC(struct hashmap_GC *t, uint64_t hash);

#endif /* HASHMAP_GC_H */
