/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hashmap.h"

#include "../../../memory.h"
#include <gc/gc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct hashmap_GC *createHashmap_GC() {
  size_t size = 8;
  struct hashmap_GC *t =
      (struct hashmap_GC *)ar_alloc(sizeof(struct hashmap_GC));
  t->size = size;
  t->order = 1;
  t->list = (struct node_GC **)ar_alloc(sizeof(struct node_GC *) * size);
  memset(t->list, 0, sizeof(struct node_GC *) * size);
  t->count = 0;
  return t;
}

void clear_hashmap_GC(struct hashmap_GC *t) {
  t->order = 1;
  t->count = 0;
  memset(t->list, 0, sizeof(struct node_GC *) * t->size);
}

void resize_hashmap_GC(struct hashmap_GC *t) {
  int old_size = t->size;
  int new_size = old_size * 2;

  struct node_GC **old_list = t->list;

  // Create new list
  t->list = (struct node_GC **)ar_alloc(sizeof(struct node_GC *) * new_size);
  memset(t->list, 0, sizeof(struct node_GC *) * new_size);

  t->size = new_size;
  t->count = 0;

  // Rehash old entries into new list
  for (int i = 0; i < old_size; i++) {
    struct node_GC *temp = old_list[i];
    while (temp) {
      hashmap_insert_GC(t, temp->hash, temp->key, temp->val,
                        temp->order); // Will increment count
      temp = temp->next;
    }
  }
}

int hashCode_GC(struct hashmap_GC *t, uint64_t hash) {
  return hash & (t->size - 1);
}

int hashmap_remove_GC(struct hashmap_GC *t, uint64_t hash) {
  int pos = hashCode_GC(t, hash);
  struct node_GC *list = t->list[pos];
  struct node_GC *temp = list;
  struct node_GC *prev = NULL;
  while (temp) {
    if (temp->hash == hash) {
      if (prev)
        prev->next = temp->next;
      else
        t->list[pos] = temp->next;
      return 1;
    }
    prev = temp;
    temp = temp->next;
  }
  list = NULL;
  prev = NULL;
  temp = NULL;
  return 0;
}

void hashmap_insert_GC(struct hashmap_GC *t, uint64_t hash, void *key,
                       void *val, size_t order) {
  if (!order) {
    order = t->order++;
  }
  if ((t->count + 1) > t->size * 0.75) {
    resize_hashmap_GC(t);
  }

  int pos = hashCode_GC(t, hash);
  struct node_GC *list = t->list[pos];
  struct node_GC *temp = list;

  // Check if key exists → overwrite
  while (temp) {
    if (temp->hash == hash) {
      temp->val = val;
      return;
    }
    temp = temp->next;
  }

  // Insert new node
  struct node_GC *newNode = (struct node_GC *)ar_alloc(sizeof(struct node_GC));
  newNode->hash = hash;
  newNode->key = key;
  newNode->val = val;
  newNode->order = order;
  newNode->next = list;
  t->list[pos] = newNode;
  t->count++;
}

void *hashmap_lookup_GC(struct hashmap_GC *t, uint64_t hash) {
  int pos = hashCode_GC(t, hash);
  struct node_GC *list = t->list[pos];
  struct node_GC *temp = list;
  while (temp) {
    if (temp->hash == hash) {
      return temp->val;
    }
    temp = temp->next;
  }
  return NULL;
}