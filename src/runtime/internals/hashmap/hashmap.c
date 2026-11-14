/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hashmap.h"
#include "../../../memory.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===========================
   Internal helpers (NO LOCKS)
   =========================== */

static inline int hashCode_GC_nolock(struct hashmap_GC *t, uint64_t hash) {
  return t->size ? (hash & (t->size - 1)) : 0;
}

static void hashmap_insert_GC_nolock(struct hashmap_GC *t, uint64_t hash,
                                     void *key, void *val, size_t order);

// Reference counting (see src/gc/README.md): a hashmap is a counted holder
// for whichever of its keys / values are managed objects (gc_flags).
static inline void count_key(struct hashmap_GC *t, void *key) {
  if (t->gc_flags & HASHMAP_GC_OBJECT_KEYS)
    gc_incref(key);
}
static inline void count_val(struct hashmap_GC *t, void *val) {
  if (t->gc_flags & HASHMAP_GC_OBJECT_VALUES)
    gc_incref(val);
}
static inline void release_node(struct hashmap_GC *t, struct node_GC *n) {
  if (t->gc_flags & HASHMAP_GC_OBJECT_KEYS)
    gc_decref(n->key);
  if (t->gc_flags & HASHMAP_GC_OBJECT_VALUES)
    gc_decref(n->val);
}
// Overwrite the value of an existing entry (the key stays).
static inline void replace_val(struct hashmap_GC *t, struct node_GC *n,
                               void *val) {
  void *old = n->val;
  count_val(t, val);
  n->val = val;
  if (t->gc_flags & HASHMAP_GC_OBJECT_VALUES)
    gc_decref(old);
}

void hashmap_finalizer(void *obj, void *client_data) {
  (void)client_data;
  struct hashmap_GC *hashmap = obj;
  RWLOCK_DESTROY(&hashmap->lock);
}

static int compare_node_asc(const void *a, const void *b) {
  const struct node_GC *na = *(const struct node_GC **)a;
  const struct node_GC *nb = *(const struct node_GC **)b;

  // Ascending order (smallest order first)
  if (na->order < nb->order)
    return -1;
  if (na->order > nb->order)
    return 1;
  return 0;
}

static void resize_hashmap_GC_nolock(struct hashmap_GC *t) {
  int old_size = t->size;
  int new_size = old_size ? old_size * 2 : 8;

  struct node_GC **old_list = t->list;

  t->list = ar_alloc(sizeof(struct node_GC *) * new_size);
  memset(t->list, 0, sizeof(struct node_GC *) * new_size);
  t->size = new_size;

  if (!old_list)
    return;

  for (int i = 0; i < old_size; i++) {
    struct node_GC *lo = NULL;
    struct node_GC *lo_tail = NULL;

    struct node_GC *hi = NULL;
    struct node_GC *hi_tail = NULL;

    struct node_GC *n = old_list[i];

    while (n) {
      struct node_GC *next = n->next;

      if ((n->hash & old_size) == 0) {
        if (!lo)
          lo = n;
        else
          lo_tail->next = n;
        lo_tail = n;
      } else {
        if (!hi)
          hi = n;
        else
          hi_tail->next = n;
        hi_tail = n;
      }

      n = next;
    }

    if (lo_tail)
      lo_tail->next = NULL;

    if (hi_tail)
      hi_tail->next = NULL;

    t->list[i] = lo;
    t->list[i + old_size] = hi;
  }
}

static void hashmap_insert_GC_nolock(struct hashmap_GC *t, uint64_t hash,
                                     void *key, void *val, size_t order) {
  if (!order)
    order = t->order++;

  /* Inline storage */
  for (size_t i = 0; i < t->inline_count; i++) {
    if (t->inline_values[i].hash == hash) {
      replace_val(t, &t->inline_values[i], val);
      return;
    }
  }

  if (!t->list && t->inline_count < INLINE_HASHMAP_ARRAY_SIZE) {
    count_key(t, key);
    count_val(t, val);
    t->inline_values[t->inline_count++] = (struct node_GC){
        .hash = hash, .key = key, .val = val, .order = order, .next = NULL};
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
      replace_val(t, temp, val);
      return;
    }
    temp = temp->next;
  }

  count_key(t, key);
  count_val(t, val);
  struct node_GC *n = ar_alloc(sizeof(struct node_GC));
  *n = (struct node_GC){.hash = hash,
                        .key = key,
                        .val = val,
                        .order = order,
                        .next = t->list[pos]};

  t->list[pos] = n;
  t->hashmap_count++;
  t->count++;
}

/* ===========================
   Public API (LOCKED)
   =========================== */

struct hashmap_GC *createHashmap_GC(void) {
  return createHashmap_GC_with(HASHMAP_GC_OBJECTS);
}

struct hashmap_GC *createHashmap_GC_with(uint8_t gc_flags) {
  struct hashmap_GC *t = ar_alloc(sizeof(struct hashmap_GC));
  memset(t, 0, sizeof(*t));
  t->order = 1;
  t->gc_flags = gc_flags;

  RWLOCK_CREATE(&t->lock);  
  // GC_register_finalizer(t, hashmap_finalizer, NULL, NULL, NULL);
  return t;
}

void clear_hashmap_GC(struct hashmap_GC *t) {
  if (!t) return;
  RWLOCK_WRLOCK(t->lock, {
    for (size_t i = 0; i < t->inline_count; i++)
      release_node(t, &t->inline_values[i]);
    if (t->list)
      for (size_t i = 0; i < (size_t)t->size; i++)
        for (struct node_GC *n = t->list[i]; n; n = n->next)
          release_node(t, n);

    t->order = 1;
    t->count = 0;
    t->inline_count = 0;
    t->hashmap_count = 0;

    if (t->list)
      memset(t->list, 0, sizeof(struct node_GC *) * t->size);
  });
}

struct node_GC **hashmap_GC_to_array(struct hashmap_GC *t,
                                     size_t *array_length) {
  RWBLOCK size_t cap = 8;
  *array_length = 0;
  RWBLOCK struct node_GC **arr = ar_alloc(cap * sizeof(*arr));

  RWLOCK_RDLOCK(t->lock, {
    // Inline values
    for (size_t i = 0; i < t->inline_count; i++) {
      if (*array_length == cap) {
        cap *= 2;
        arr = ar_realloc(arr, cap * sizeof(*arr));
      }
      arr[(*array_length)++] = &t->inline_values[i];
    }

    // Hash list
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
  });

  qsort(arr, *array_length, sizeof(struct node_GC *), compare_node_asc);

  return arr;
}

int hashmap_remove_GC(struct hashmap_GC *t, uint64_t hash) {
  if (!t) return 0;
  RWBLOCK int result = 0;

  RWLOCK_WRLOCK(t->lock, {
    // Inline values
    for (size_t i = 0; i < t->inline_count; i++) {
      if (t->inline_values[i].hash == hash) {
        release_node(t, &t->inline_values[i]);
        memmove(&t->inline_values[i], &t->inline_values[i + 1],
                (t->inline_count - i - 1) * sizeof(struct node_GC));
        t->inline_count--;
        result = 1;
        break;
      }
    }

    if (result == 0 && t->list) {
      int pos = hashCode_GC_nolock(t, hash);
      struct node_GC *prev = NULL;
      struct node_GC *cur = t->list[pos];

      while (cur) {
        if (cur->hash == hash) {
          release_node(t, cur);
          if (prev)
            prev->next = cur->next;
          else
            t->list[pos] = cur->next;
          result = 1;
          break;
        }
        prev = cur;
        cur = cur->next;
      }
    }
  });

  return result;
}

void hashmap_insert_GC(struct hashmap_GC *t, uint64_t hash, void *key,
                       void *val, size_t order) {
  RWLOCK_WRLOCK(t->lock,
                { hashmap_insert_GC_nolock(t, hash, key, val, order); });
}


void *hashmap_lookup_node_GC(struct hashmap_GC *t, uint64_t hash) {
  if (!t) return NULL;
  RWBLOCK struct node_GC *result = NULL;

  RWLOCK_RDLOCK(t->lock, {
    for (size_t i = 0; i < t->inline_count; i++) {
      if (t->inline_values[i].hash == hash) {
        result = &t->inline_values[i];
        break;
      }
    }

    if (!result && t->list) {
      int pos = hashCode_GC_nolock(t, hash);
      struct node_GC *n = t->list[pos];

      while (n) {
        if (n->hash == hash) {
          result = n;
          break;
        }
        n = n->next;
      }
    }
  });

  return result;
}


void *hashmap_lookup_GC(struct hashmap_GC *t, uint64_t hash) {
  if (!t) return NULL;
  RWBLOCK void *result = NULL;

  RWLOCK_RDLOCK(t->lock, {
    for (size_t i = 0; i < t->inline_count; i++) {
      if (t->inline_values[i].hash == hash) {
        result = t->inline_values[i].val;
        break;
      }
    }

    if (!result && t->list) {
      int pos = hashCode_GC_nolock(t, hash);
      struct node_GC *n = t->list[pos];

      while (n) {
        if (n->hash == hash) {
          result = n->val;
          break;
        }
        n = n->next;
      }
    }
  });

  return result;
}

static inline void visit_node(struct hashmap_GC *t, struct node_GC *n,
                              gc_visit_fn visit, void *ctx) {
  if (t->gc_flags & HASHMAP_GC_OBJECT_KEYS)
    visit(n->key, ctx);
  if (t->gc_flags & HASHMAP_GC_OBJECT_VALUES)
    visit(n->val, ctx);
}

// No lock: the collector only runs at safe points (single-threaded for now).
void hashmap_GC_traverse(struct hashmap_GC *t, gc_visit_fn visit, void *ctx) {
  if (!t || !t->gc_flags)
    return;
  for (size_t i = 0; i < t->inline_count; i++)
    visit_node(t, &t->inline_values[i], visit, ctx);
  if (!t->list)
    return;
  for (size_t i = 0; i < t->size; i++)
    for (struct node_GC *n = t->list[i]; n; n = n->next)
      visit_node(t, n, visit, ctx);
}

void hashmap_GC_for_each_value(struct hashmap_GC *t, gc_visit_fn visit,
                               void *ctx) {
  if (!t)
    return;
  for (size_t i = 0; i < t->inline_count; i++)
    visit(t->inline_values[i].val, ctx);
  if (!t->list)
    return;
  for (size_t i = 0; i < t->size; i++)
    for (struct node_GC *n = t->list[i]; n; n = n->next)
      visit(n->val, ctx);
}

void hashmap_GC_claim(struct hashmap_GC *t, void *holder) {
  if (!t)
    return;
  if (!t->gc_owner)
    t->gc_owner = holder;
  else if (t->gc_owner != (GCHeader *)holder)
    gc_incref(t->gc_owner); // a view keeps the owner (and so the map) alive
}

void hashmap_GC_traverse_from(struct hashmap_GC *t, void *holder,
                              gc_visit_fn visit, void *ctx) {
  if (!t)
    return;
  if (!t->gc_owner || t->gc_owner == (GCHeader *)holder)
    hashmap_GC_traverse(t, visit, ctx);
  else
    visit(t->gc_owner, ctx);
}
