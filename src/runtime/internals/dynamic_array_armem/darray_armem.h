/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef darray_armem_H
#define darray_armem_H

#include <stdbool.h>
#include <stddef.h> // for size_t

#define CHUNK_SIZE 16

typedef struct {
  void *data;
  size_t element_size;
  size_t size;
  size_t capacity;
} darray_armem;

// Initializes the dynamic_array
void darray_armem_init(darray_armem *arr, size_t element_size);

// Pushes an element onto the array
void darray_armem_push(darray_armem *arr, void *element);

// Pops the last element, calling `free_data` if provided
void darray_armem_pop(darray_armem *arr, void (*free_data)(void *));

// Gets a pointer to an element at index
void *darray_armem_get(darray_armem *arr, size_t index);

// Resizes the array to a new size (internal use, but exposed)
void darray_armem_resize(darray_armem *arr, size_t new_size);

darray_armem darray_armem_slice(darray_armem *arr, size_t start, size_t end);

#endif // darray_armem_H
