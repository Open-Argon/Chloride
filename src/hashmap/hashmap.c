#include "hashmap.h"

#include <stdlib.h>
#include "../memory.h"

struct table *createTable(int size)
{
    struct table *t = (struct table *)checked_malloc(sizeof(struct table));
    t->size = size;
    t->list = (struct node **)checked_malloc(sizeof(struct node *) * size);
    int i;
    for (i = 0; i < size; i++)
        t->list[i] = NULL;
    return t;
}

int hashCode(struct table *t, int key)
{
    if (key < 0)
        return -(key % t->size);
    return key % t->size;
}

int remove(struct table *t, int key)
{
    int pos = hashCode(t, key);
    struct node *list = t->list[pos];
    struct node *temp = list;
    struct node *prev = NULL;
    while (temp)
    {
        if (temp->key == key)
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

void insert(struct table *t, int key, void* val)
{
    int pos = hashCode(t, key);
    struct node *list = t->list[pos];
    struct node *newNode = (struct node *)checked_malloc(sizeof(struct node));
    struct node *temp = list;
    while (temp)
    {
        if (temp->key == key)
        {
            temp->val = val;
            return;
        }
        temp = temp->next;
    }
    newNode->key = key;
    newNode->val = val;
    newNode->next = list;
    t->list[pos] = newNode;
}

void *lookup(struct table *t, int key)
{
    int pos = hashCode(t, key);
    struct node *list = t->list[pos];
    struct node *temp = list;
    while (temp)
    {
        if (temp->key == key)
        {
            return temp->val;
        }
        temp = temp->next;
    }
    return NULL;
}