#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdint.h>
#include <stdlib.h>

typedef struct ArgonObject ArgonObject;

struct node
{
    uint64_t hash;
    ArgonObject *key;
    ArgonObject *val;
    struct node *next;
};
struct hashmap
{
    size_t size;
    size_t count;
    struct node **list;
};

struct hashmap *createHashmap(int size);

int hashCode(struct hashmap *t, uint64_t hash);

int hashmap_remove(struct hashmap *t, uint64_t hash);

void hashmap_insert(struct hashmap *t, uint64_t  hash, ArgonObject* key, ArgonObject* val);

#endif // HASHMAP_H