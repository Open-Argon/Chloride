#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdlib.h>

struct node
{
    size_t key;
    void *val;
    struct node *next;
};
struct table
{
    size_t size;
    size_t count;
    struct node **list;
};

struct table *createTable(int size);

int hashCode(struct table *t, int key);

int remove(struct table *t, int key);

void insert(struct table *t, int key, void* val);

#endif // HASHMAP_H