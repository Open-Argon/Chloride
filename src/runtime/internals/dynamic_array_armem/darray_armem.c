#include "darray_armem.h"
#include "../../../memory.h"
#include <gc/gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void darray_armem_init(darray_armem *arr, size_t element_size) {
  arr->element_size = element_size;
  arr->size = 0;
  arr->capacity = CHUNK_SIZE;
  arr->data = ar_alloc(CHUNK_SIZE * element_size);
  arr->resizable = true;
  if (!arr->data) {
    fprintf(stderr, "darray_armem_init: allocation failed\n");
    exit(EXIT_FAILURE);
  }
}

void darray_armem_resize(darray_armem *arr, size_t new_size) {
  if (!arr->resizable) {
    fprintf(stderr, "darray_armem_resize: unresizable darray_armem\n");
    exit(EXIT_FAILURE);
  }
  size_t new_capacity = ((new_size + CHUNK_SIZE) / CHUNK_SIZE) * CHUNK_SIZE;
  if (new_capacity != arr->capacity) {
    void *new_data = ar_alloc(new_capacity * arr->element_size);
    memccpy(new_data,arr->data, arr->element_size, arr->capacity);
    if (!new_data) {
      fprintf(stderr, "darray_armem_resize: reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    arr->data = new_data;
    arr->capacity = new_capacity;
  }

  arr->size = new_size;
}

void darray_armem_push(darray_armem *arr, void *element) {
  if (!arr->resizable) {
    fprintf(stderr, "darray_armem_resize: unresizable darray_armem\n");
    exit(EXIT_FAILURE);
  }
  if (arr->size >= arr->capacity) {
    darray_armem_resize(arr, arr->size + 1);
  } else {
    arr->size++;
  }

  void *target = (char *)arr->data + (arr->size - 1) * arr->element_size;
  memcpy(target, element, arr->element_size);
}

void darray_armem_pop(darray_armem *arr, void (*free_data)(void *)) {
  if (!arr->resizable) {
    fprintf(stderr, "darray_armem_resize: unresizable darray_armem\n");
    exit(EXIT_FAILURE);
  }
  if (arr->size == 0)
    return;

  if (free_data) {
    void *target = (char *)arr->data + (arr->size-1) * arr->element_size;
    free_data(target);
  }
  
  darray_armem_resize(arr, arr->size-1);
}

void *darray_armem_get(darray_armem *arr, size_t index) {
  if (index >= arr->size) {
    fprintf(stderr, "darray_armem_get: index out of bounds\n");
    exit(EXIT_FAILURE);
  }
  return (char *)arr->data + index * arr->element_size;
}

darray_armem darray_armem_slice(darray_armem *arr, size_t start, size_t end) {
  if (start > end || end > arr->size) {
    fprintf(stderr, "darray_armem_slice: invalid slice range\n");
    exit(EXIT_FAILURE);
  }

  darray_armem slice;

  slice.data = (char *)arr->data + start * arr->element_size;
  slice.size = (end - start);
  slice.element_size = arr->element_size;
  slice.capacity = ((slice.size + CHUNK_SIZE) / CHUNK_SIZE) * CHUNK_SIZE;
  slice.resizable = false;

  return slice;
}

void darray_armem_free(darray_armem *arr, void (*free_data)(void *)) {
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