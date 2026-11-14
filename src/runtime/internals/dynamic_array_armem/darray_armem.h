/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef darray_armem_H
#define darray_armem_H

#include <stdbool.h>
#include <stddef.h>
#include "../../../RWLock.h"
#include "../../../gc/gc.h"

#define DARRAY_ARMEM_CHUNK_SIZE 128

typedef struct {
  void *data;
  size_t element_size;
  size_t size;
  size_t capacity;
  size_t offset;          // space at the start for efficient pops
  RWLock lock;
  // Elements are ArgonObject * and the array is a counted holder of them
  // (see src/gc/README.md): insert/pop/resize/slice incref and decref.
  bool gc_objects;
} darray_armem;

darray_armem* darray_armem_create();

// Initialize the dynamic array
void darray_armem_init(darray_armem *arr, size_t element_size, size_t initial_size);

// Initialize an array of ArgonObject * that counts its elements. The
// initial_size elements start out NULL.
void darray_armem_init_objects(darray_armem *arr, size_t initial_size);

// Free the element buffer (not the elements, and not `arr` itself).
void darray_armem_free_data(darray_armem *arr);

// Element `index` of an object array, as an lvalue-friendly pointer.
#define DARRAY_OBJECT_SLOT(arr, index)                                         \
  ((ArgonObject **)darray_armem_get((arr), (index)))

// Store into element `index` of an object array: increfs the new value and
// decrefs the old one.
#define darray_armem_set_object(arr, index, value)                             \
  gc_assign(DARRAY_OBJECT_SLOT((arr), (index)), (value))

// Insert element at position (size = append)
void darray_armem_insert(darray_armem *arr, size_t pos, void *element);

// Pop element at position (size-1 = default pop)
bool darray_armem_pop(darray_armem *arr, size_t pos, void*out);

// Get element at index (read-only)
void *darray_armem_get(darray_armem *arr, size_t index);

// Resize array to new size (internal)
void darray_armem_resize(darray_armem *arr, size_t new_size);

// Return a slice of the array
darray_armem darray_armem_slice(darray_armem *arr, size_t start, size_t end);

#endif // darray_armem_H
