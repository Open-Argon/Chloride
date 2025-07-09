#ifndef HASHMAP_GC_H
#define HASHMAP_GC_H
#include <stdint.h>
#include <stdlib.h>

struct node_GC {
  uint64_t hash;
  void *key;
  void *val;
  size_t order;
  struct node_GC *next;
};
struct hashmap_GC {
  size_t size;
  size_t count;
  size_t order;
  struct node_GC **list;
};

struct hashmap_GC *createHashmap_GC();

int hashCode_GC(struct hashmap_GC *t, uint64_t hash);

int hashmap_remove_GC(struct hashmap_GC *t, uint64_t hash);

void hashmap_insert_GC(struct hashmap_GC *t, uint64_t hash, void *key,
                    void *val, size_t order);

void *hashmap_lookup_GC(struct hashmap_GC *t, uint64_t hash);

#endif // HASHMAP_GC_H