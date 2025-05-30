#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h>  // for size_t

#define CHUNK_SIZE 16

typedef struct {
    void *data;
    size_t element_size;
    size_t size;
    size_t capacity;
} DArray;

// Initializes the dynamic_array
void darray_init(DArray *arr, size_t element_size);

// Pushes an element onto the array
void darray_push(DArray *arr, void *element);

// Pops the last element, calling `free_data` if provided
void darray_pop(DArray *arr, void (*free_data)(void *));

// Gets a pointer to an element at index
void *darray_get(DArray *arr, size_t index);

// Frees the entire array and optionally each element
void darray_free(DArray *arr, void (*free_data)(void *));

// Resizes the array to a new size (internal use, but exposed)
void darray_resize(DArray *arr, size_t new_size);

#endif // DARRAY_H
