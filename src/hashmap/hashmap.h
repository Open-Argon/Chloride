#ifndef HASHMAP_H
#define HASHMAP_H

struct node
{
    int key;
    void *val;
    struct node *next;
};
struct table
{
    int size;
    struct node **list;
};

struct table *createTable(int size);

int hashCode(struct table *t, int key);

int remove(struct table *t, int key);

void insert(struct table *t, int key, void* val);

#endif // HASHMAP_H