#include "hashmap.h"

#include <stdint.h>
#include <gc/gc.h>
#include <stdlib.h>
#include "../../../memory.h"

struct hashmap *createHashmap(int size)
{
    struct hashmap *t = (struct hashmap *)ar_alloc(sizeof(struct hashmap));
    t->size = size;
    t->list = (struct node **)ar_alloc(sizeof(struct node *) * size);
    int i;
    for (i = 0; i < size; i++)
        t->list[i] = NULL;
    return t;
}

void resize_hashmap(struct hashmap *t)
{
    int old_size = t->size;
    int new_size = old_size * 2;

    struct node **old_list = t->list;

    // Create new list
    t->list = (struct node **)ar_alloc(sizeof(struct node *) * new_size);
    for (int i = 0; i < new_size; i++)
        t->list[i] = NULL;

    t->size = new_size;
    t->count = 0;

    // Rehash old entries into new list
    for (int i = 0; i < old_size; i++) {
        struct node *temp = old_list[i];
        while (temp) {
            hashmap_insert(t, temp->hash, temp->key, temp->val); // Will increment count
            temp = temp->next;
        }
    }
}

int hashCode(struct hashmap *t, uint64_t hash)
{
    return hash % t->size;
}

int hashmap_remove(struct hashmap *t, uint64_t hash)
{
    int pos = hashCode(t, hash);
    struct node *list = t->list[pos];
    struct node *temp = list;
    struct node *prev = NULL;
    while (temp)
    {
        if (temp->hash == hash)
        {
            if (prev)
                prev->next = temp->next;
            else
                t->list[pos] = temp->next;

            free(temp);
            return 1;
        }
        prev = temp;
        temp = temp->next;
    }
    return 0;
}

void resize(struct hashmap *t)
{
    int old_size = t->size;
    int new_size = old_size * 2;

    struct node **old_list = t->list;

    // Create new list
    t->list = (struct node **)ar_alloc(sizeof(struct node *) * new_size);
    for (int i = 0; i < new_size; i++)
        t->list[i] = NULL;

    t->size = new_size;
    t->count = 0;

    // Rehash old entries into new list
    for (int i = 0; i < old_size; i++) {
        struct node *temp = old_list[i];
        while (temp) {
            hashmap_insert(t, temp->hash, temp->key, temp->val); // Will increment count
            temp = temp->next;
        }
    }
}

void hashmap_insert(struct hashmap *t, uint64_t  hash, ArgonObject* key, ArgonObject* val)
{
    if ((t->count + 1) > t->size * 0.75) {
        resize(t);
    }

    int pos = hashCode(t, hash);
    struct node *list = t->list[pos];
    struct node *temp = list;

    // Check if key exists → overwrite
    while (temp)
    {
        if (temp->key == key)
        {
            temp->val = val;
            return;
        }
        temp = temp->next;
    }

    // Insert new node
    struct node *newNode = (struct node *)ar_alloc(sizeof(struct node));
    newNode->hash = hash;
    newNode->key = key;
    newNode->val = val;
    newNode->next = list;
    t->list[pos] = newNode;
    t->count++;
}

void *lookup(struct hashmap *t, uint64_t hash)
{
    int pos = hashCode(t, hash);
    struct node *list = t->list[pos];
    struct node *temp = list;
    while (temp)
    {
        if (temp->hash == hash)
        {
            return temp->val;
        }
        temp = temp->next;
    }
    return NULL;
}