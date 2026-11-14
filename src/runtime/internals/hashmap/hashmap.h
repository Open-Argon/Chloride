/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HASHMAP_GC_H
#define HASHMAP_GC_H

#include <stdbool.h>
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

  // The managed object this map belongs to (a Stack, an object's attribute
  // dict, a dictionary), or NULL. See hashmap_GC_claim.
  GCHeader *gc_owner;
};

#define HASHMAP_GC_OBJECT_KEYS 1
#define HASHMAP_GC_OBJECT_VALUES 2
#define HASHMAP_GC_OBJECTS (HASHMAP_GC_OBJECT_KEYS | HASHMAP_GC_OBJECT_VALUES)
// Keys are buffers the map owns (from ar_alloc); freed with their entry.
#define HASHMAP_GC_OWNS_KEYS 4

/* Public API (thread-safe) */

// Keys and values are both managed objects (or NULL): the usual case.
struct hashmap_GC *createHashmap_GC(void);
// Same, but say which of keys/values are managed objects (HASHMAP_GC_*).
struct hashmap_GC *createHashmap_GC_with(uint8_t gc_flags);

// Visit every key and value that is a managed object (see gc_flags).
void hashmap_GC_traverse(struct hashmap_GC *t, gc_visit_fn visit, void *ctx);

// A hashmap belongs to exactly one managed object, its owner: the first to
// claim it. Anyone else that claims it afterwards (e.g. the dictionary
// `import` returns, which is a live view of the module's scope) is a *view*:
// it holds a counted reference to the owner rather than to the entries, so
// the entries are only ever counted, traversed and freed once.
// Returns true if `holder` is (now) the owner.
bool hashmap_GC_claim(struct hashmap_GC *t, void *holder);

// What `holder` should traverse for the map it holds: the entries if it owns
// the map, or just the owner if it's a view.
void hashmap_GC_traverse_from(struct hashmap_GC *t, void *holder,
                              gc_visit_fn visit, void *ctx);

// Visit every value, whatever gc_flags says (for maps of uncounted roots).
void hashmap_GC_for_each_value(struct hashmap_GC *t, gc_visit_fn visit,
                               void *ctx);

void clear_hashmap_GC(struct hashmap_GC *t);

// A snapshot of the entries in insertion order. The nodes are copies, so the
// snapshot stays valid even if the map changes (or is freed) meanwhile; its
// keys and values aren't counted, so hold on to the map while using it.
// Release it with hashmap_GC_free_array.
struct node_GC **hashmap_GC_to_array(
    struct hashmap_GC *t,
    size_t *array_length
);
void hashmap_GC_free_array(struct node_GC **array);

// Free the map's own memory (entries, buckets, owned keys, the struct). Does
// NOT decref the keys/values: for GC finalizers, where the collector has
// already released them through traverse.
void hashmap_GC_free(struct hashmap_GC *t);

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
