#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdlib.h>

struct node
{
    size_t key;
    void *val;
    struct node *next;
};
struct hashmap
{
    size_t size;
    size_t count;
    struct node **list;
};

struct hashmap *createTable(int size);

int hashCode(struct hashmap *t, int key);

int remove(struct hashmap *t, int key);

void insert(struct hashmap *t, int key, void* val);

#endif // HASHMAP_H