#include "darray.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void darray_init(DArray *arr, size_t element_size) {
  arr->element_size = element_size;
  arr->size = 0;
  arr->capacity = CHUNK_SIZE;
  arr->data = malloc(CHUNK_SIZE * element_size);
  arr->resizable = true;
  if (!arr->data) {
    fprintf(stderr, "darray_init: allocation failed\n");
    exit(EXIT_FAILURE);
  }
}

void darray_resize(DArray *arr, size_t new_size) {
  if (!arr->resizable) {
    fprintf(stderr, "darray_resize: unresizable darray\n");
    exit(EXIT_FAILURE);
  }
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
  if (!arr->resizable) {
    fprintf(stderr, "darray_resize: unresizable darray\n");
    exit(EXIT_FAILURE);
  }
  if (arr->size >= arr->capacity) {
    darray_resize(arr, arr->size + 1);
  } else {
    arr->size++;
  }

  void *target = (char *)arr->data + (arr->size - 1) * arr->element_size;
  memcpy(target, element, arr->element_size);
}

void darray_pop(DArray *arr, void (*free_data)(void *)) {
  if (!arr->resizable) {
    fprintf(stderr, "darray_resize: unresizable darray\n");
    exit(EXIT_FAILURE);
  }
  if (arr->size == 0)
    return;

  if (free_data) {
    void *target = (char *)arr->data + (arr->size-1) * arr->element_size;
    free_data(target);
  }
  
  darray_resize(arr, arr->size-1);
}

void *darray_get(DArray *arr, size_t index) {
  if (index >= arr->size) {
    fprintf(stderr, "darray_get: index out of bounds\n");
    exit(EXIT_FAILURE);
  }
  return (char *)arr->data + index * arr->element_size;
}

DArray darray_slice(DArray *arr, size_t start, size_t end) {
  if (start > end || end > arr->size) {
    fprintf(stderr, "darray_slice: invalid slice range\n");
    exit(EXIT_FAILURE);
  }

  DArray slice;

  slice.data = (char *)arr->data + start * arr->element_size;
  slice.size = (end - start);
  slice.element_size = arr->element_size;
  slice.capacity = ((slice.size + CHUNK_SIZE) / CHUNK_SIZE) * CHUNK_SIZE;
  slice.resizable = false;

  return slice;
}

void darray_free(DArray *arr, void (*free_data)(void *)) {
  if (!arr->resizable) {
    // It's a view/slice — don't free
    return;
  }
  if (free_data) {
    for (size_t i = 0; i < arr->size; ++i) {
      void *element = (char *)arr->data + i * arr->element_size;
      free_data(element);
    }
  }
  free(arr->data);
  arr->data = NULL;
  arr->size = 0;
  arr->capacity = 0;
  arr->element_size = 0;
  arr->resizable = false;
}