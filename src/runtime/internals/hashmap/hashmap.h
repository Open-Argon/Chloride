/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HASHMAP_GC_H
#define HASHMAP_GC_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
  struct node_GC inline_values[INLINE_HASHMAP_ARRAY_SIZE];
  size_t count;
  size_t inline_count;
  size_t hashmap_count;
  size_t order;
  struct node_GC **list;
};

struct hashmap_GC *createHashmap_GC();

void clear_hashmap_GC(struct hashmap_GC *t);

void hashmap_GC_to_array(struct hashmap_GC *t, struct node_GC**array,
                           size_t *array_length);

int hashCode_GC(struct hashmap_GC *t, uint64_t hash);

int hashmap_remove_GC(struct hashmap_GC *t, uint64_t hash);

void hashmap_insert_GC(struct hashmap_GC *t, uint64_t hash, void *key,
                       void *val, size_t order);

void *hashmap_lookup_GC(struct hashmap_GC *t, uint64_t hash);

#endif // HASHMAP_GC_H