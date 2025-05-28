#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h> // for size_t

// Node structure (opaque to user)
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *head;
    size_t data_size;
    size_t length;
} LinkedList;

// Create a new list for the given data type size
LinkedList *create_list(size_t data_size);

// Append an element to the list
void append(LinkedList *list, void *element);

// Get a pointer to the element at the given index
void *get_element_at(LinkedList *list, size_t index);

// Set the element at the given index
int set_element_at(LinkedList *list, size_t index, void *element);

// Remove the element at the given index
int remove_at(LinkedList *list, size_t index);

// Get the number of elements in the list
size_t list_length(LinkedList *list);

// Print the list using a provided print function
void print_list(LinkedList *list, void (*print_func)(void *));

// Free all memory used by the list
void free_list(LinkedList *list);

#endif // LINKEDLIST_H