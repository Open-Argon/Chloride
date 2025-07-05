#include "hashmap.h"
#include "../memory.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct hashmap *createHashmap() {
  size_t size = 8;
  struct hashmap *t = (struct hashmap *)checked_malloc(sizeof(struct hashmap));
  t->size = size;
  t->order = 1;
  t->list = (struct node **)checked_malloc(sizeof(struct node *) * size);
  memset(t->list, 0, sizeof(struct node *) * size);
  return t;
}

void hashmap_free(struct hashmap *t, free_val_func free_val) {
  if (!t)
    return;

  for (size_t i = 0; i < t->size; i++) {
    struct node *current = t->list[i];
    while (current) {
      struct node *next = current->next;
      if (free_val && current->val) {
        free_val(current->val);
      }
      free(current);
      current = next;
    }
  }
  free(t->list);
  free(t);
}

void resize_hashmap(struct hashmap *t) {
  int old_size = t->size;
  int new_size = old_size * 2;

  struct node **old_list = t->list;

  // Create new list
  t->list = (struct node **)checked_malloc(sizeof(struct node *) * new_size);
  memset(t->list, 0, sizeof(struct node *) * new_size);

  t->size = new_size;
  t->count = 0;

  // Rehash old entries into new list
  for (int i = 0; i < old_size; i++) {
    struct node *temp = old_list[i];
    while (temp) {
      hashmap_insert(t, temp->hash, temp->key, temp->val,
                     temp->order); // Will increment count
      temp = temp->next;
    }
  }
  free(old_list);
}

int hashCode(struct hashmap *t, uint64_t hash) { return hash & (t->size - 1); }

int hashmap_remove(struct hashmap *t, uint64_t hash) {
  int pos = hashCode(t, hash);
  struct node *list = t->list[pos];
  struct node *temp = list;
  struct node *prev = NULL;
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

void hashmap_insert(struct hashmap *t, uint64_t hash, void *key, void *val,
                    size_t order) {
  if (!order) {
    order = t->order++;
  }
  if ((t->count + 1) > t->size * 0.75) {
    resize_hashmap(t);
  }

  int pos = hashCode(t, hash);
  struct node *list = t->list[pos];
  struct node *temp = list;

  // Check if key exists → overwrite
  while (temp) {
    if (temp->hash == hash) {
      temp->val = val;
      return;
    }
    temp = temp->next;
  }

  // Insert new node
  struct node *newNode = (struct node *)checked_malloc(sizeof(struct node));
  newNode->hash = hash;
  newNode->key = key;
  newNode->val = val;
  newNode->order = order;
  newNode->next = list;
  t->list[pos] = newNode;
  t->count++;
}

void *hashmap_lookup(struct hashmap *t, uint64_t hash) {
  int pos = hashCode(t, hash);
  struct node *list = t->list[pos];
  struct node *temp = list;
  while (temp) {
    if (temp->hash == hash) {
      return temp->val;
    }
    temp = temp->next;
  }
  return NULL;
}