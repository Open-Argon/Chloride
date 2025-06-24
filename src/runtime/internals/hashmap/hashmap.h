#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdint.h>
#include <stdlib.h>

typedef struct ArgonObject ArgonObject;

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

int hashCode(struct hashmap *t, uint64_t hash);

int hashmap_remove(struct hashmap *t, uint64_t hash);

void hashmap_insert(struct hashmap *t, uint64_t hash, void *key,
                    void *val, size_t order);

void *hashmap_lookup(struct hashmap *t, uint64_t hash);

#endif // HASHMAP_H