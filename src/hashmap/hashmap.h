#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdint.h>
#include <stdlib.h>

typedef struct ArgonObject ArgonObject;

typedef void (*free_val_func)(void *val);

struct node {
  uint64_t hash;
  void *key;
  void *val;
  size_t order;
  struct node *next;
};
struct hashmap {
  size_t size;
  size_t count;
  size_t order;
  struct node **list;
};

struct hashmap *createHashmap();

void hashmap_free(struct hashmap *t, free_val_func free_val);

int hashCode(struct hashmap *t, uint64_t hash);

int hashmap_remove(struct hashmap *t, uint64_t hash);

void hashmap_insert(struct hashmap *t, uint64_t hash, void *key,
                    void *val, size_t order);

void *hashmap_lookup(struct hashmap *t, uint64_t hash);

#endif // HASHMAP_H