#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "darray.h"

void darray_init(DArray *arr, size_t element_size) {
    arr->element_size = element_size;
    arr->size = 0;
    arr->capacity = CHUNK_SIZE;
    arr->data = malloc(CHUNK_SIZE * element_size);
    if (!arr->data) {
        fprintf(stderr, "darray_init: allocation failed\n");
        exit(EXIT_FAILURE);
    }
}

void darray_resize(DArray *arr, size_t new_size) {
    size_t new_capacity = ((new_size + CHUNK_SIZE) / CHUNK_SIZE) * CHUNK_SIZE;
    if (new_capacity != arr->capacity) {
        void *new_data = realloc(arr->data, new_capacity * arr->element_size);
        if (!new_data) {
            fprintf(stderr, "darray_resize: reallocation failed\n");
            exit(EXIT_FAILURE);
        }
        arr->data = new_data;
        arr->capacity = new_capacity;
    }

    arr->size = new_size;
}

void darray_push(DArray *arr, void *element) {
    if (arr->size >= arr->capacity) {
        darray_resize(arr, arr->size + 1);
    } else {
        arr->size++;
    }

    void *target = (void *)arr->data + (arr->size - 1) * arr->element_size;
    memcpy(target, element, arr->element_size);
}

void darray_pop(DArray *arr, void (*free_data)(void *)) {
    if (arr->size == 0)
        return;

    arr->size--;

    if (free_data) {
        void *target = (void *)arr->data + arr->size * arr->element_size;
        free_data(target);
    }

    darray_resize(arr, arr->size);
}

void *darray_get(DArray *arr, size_t index) {
    if (index >= arr->size) {
        fprintf(stderr, "darray_get: index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    return (void *)arr->data + index * arr->element_size;
}

void darray_free(DArray *arr, void (*free_data)(void *)) {
    if (free_data) {
        for (size_t i = 0; i < arr->size; ++i) {
            void *element = (void *)arr->data + i * arr->element_size;
            free_data(element);
        }
    }
    free(arr->data);
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
    arr->element_size = 0;
}