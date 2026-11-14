/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "darray_armem.h"
#include "../../../memory.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void darray_armem_finalizer(void *obj, void *client_data) {
  (void)client_data;
  darray_armem *arr = obj;
  RWLOCK_DESTROY(&arr->lock);
}

darray_armem *darray_armem_create() { return ar_alloc(sizeof(darray_armem)); }

void darray_armem_init(darray_armem *arr, size_t element_size,
                       size_t initial_size) {
  if (element_size > DARRAY_ARMEM_CHUNK_SIZE) {
    fprintf(stderr, "darray_armem_init: element size larger than chunk size\n");
    exit(EXIT_FAILURE);
  }

  arr->element_size = element_size;
  arr->size = initial_size;
  arr->offset = 0;

  size_t bytes_needed = initial_size * element_size;

  // round up to chunk size
  size_t chunks =
      (bytes_needed + DARRAY_ARMEM_CHUNK_SIZE - 1) / DARRAY_ARMEM_CHUNK_SIZE;

  size_t alloc_size = chunks * DARRAY_ARMEM_CHUNK_SIZE;

  arr->capacity = alloc_size / element_size;
  arr->data = ar_alloc(alloc_size);

  if (!arr->data) {
    fprintf(stderr, "darray_armem_init: allocation failed\n");
    exit(EXIT_FAILURE);
  }

  RWLOCK_CREATE(&arr->lock);
  arr->gc_objects = false;

  // GC_register_finalizer(arr, darray_armem_finalizer, NULL, NULL, NULL);
}

void darray_armem_init_objects(darray_armem *arr, size_t initial_size) {
  darray_armem_init(arr, sizeof(struct ArgonObject *), initial_size);
  memset(arr->data, 0, arr->capacity * sizeof(struct ArgonObject *));
  arr->gc_objects = true;
}

struct ArgonObject;
static inline struct ArgonObject **object_at(darray_armem *arr, size_t index) {
  return (struct ArgonObject **)arr->data + arr->offset + index;
}

static void darray_armem_resize_nolock(darray_armem *arr, size_t new_size) {
  size_t old_size = arr->size;
  if (arr->gc_objects) // elements dropped by shrinking are released
    for (size_t i = new_size; i < old_size; i++)
      gc_decref(*object_at(arr, i));
  size_t required_bytes = (arr->offset + new_size) * arr->element_size;
  size_t new_capacity_bytes = required_bytes * 2;
  size_t new_capacity = new_capacity_bytes / arr->element_size;

  if (new_capacity && new_capacity != arr->capacity) {
    arr->data = ar_realloc(arr->data, new_capacity_bytes);
    if (!arr->data) {
      fprintf(stderr, "darray_armem_resize: reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    arr->capacity = new_capacity;
  }

  // Elements gained by growing start out NULL (realloc doesn't zero them).
  if (new_size > old_size)
    memset((char *)arr->data + (arr->offset + old_size) * arr->element_size, 0,
           (new_size - old_size) * arr->element_size);
  arr->size = new_size;
}

void darray_armem_resize(darray_armem *arr, size_t new_size) {
  RWLOCK_WRLOCK(arr->lock, { darray_armem_resize_nolock(arr, new_size); });
}

void darray_armem_insert(darray_armem *arr, size_t pos_, void *element) {
  RWBLOCK size_t pos = pos_;
  RWLOCK_WRLOCK(arr->lock, {
    if (pos > arr->size)
      pos = arr->size;

    if (arr->size >= arr->capacity) {
      darray_armem_resize_nolock(arr, arr->size + 1);
    } else {
      arr->size++;
    }

    void *base = (char *)arr->data + arr->offset * arr->element_size;
    void *target = (char *)base + pos * arr->element_size;

    if (pos < arr->size - 1) {
      memmove((char *)target + arr->element_size, target,
              (arr->size - pos - 1) * arr->element_size);
    }

    memcpy(target, element, arr->element_size);
    if (arr->gc_objects)
      gc_incref(*(struct ArgonObject **)element);
  });
}

bool darray_armem_pop(darray_armem *arr, size_t pos_, void *out) {
  RWBLOCK size_t pos = pos_;
  RWBLOCK bool result = false;

  RWLOCK_WRLOCK(arr->lock, {
    if (arr->size == 0) {
      result = false;
    } else {

      if (pos >= arr->size)
        pos = arr->size - 1;

      void *base = (char *)arr->data + arr->offset * arr->element_size;
      void *target = (char *)base + pos * arr->element_size;

      if (out) {
        memcpy(out, target, arr->element_size);
      }
      // The popped element now lives only in the caller's C local (or
      // nowhere), which isn't counted.
      if (arr->gc_objects)
        gc_decref(*(struct ArgonObject **)target);

      if (pos < arr->size - 1) {
        memmove(target, (char *)target + arr->element_size,
                (arr->size - pos - 1) * arr->element_size);
      }
      arr->size--;
      result = true;
    }
  });

  return result;
}

void *darray_armem_get(darray_armem *arr, size_t index) {
  RWBLOCK void *ptr = NULL;

  RWLOCK_RDLOCK(arr->lock, {
    if (index >= arr->size) {
      fprintf(stderr, "darray_armem_get: index out of bounds\n");
      exit(EXIT_FAILURE);
    }

    ptr = (char *)arr->data + (arr->offset + index) * arr->element_size;
  });

  return ptr;
}

darray_armem darray_armem_slice(darray_armem *arr, size_t start, size_t end) {
  RWBLOCK darray_armem slice;

  RWLOCK_RDLOCK(arr->lock, {
    if (start > end || end > arr->size) {
      fprintf(stderr, "darray_armem_slice: invalid slice range\n");
      exit(EXIT_FAILURE);
    }

    slice.size = end - start;
    slice.element_size = arr->element_size;
    slice.capacity =
        ((slice.size + DARRAY_ARMEM_CHUNK_SIZE) / DARRAY_ARMEM_CHUNK_SIZE) *
        DARRAY_ARMEM_CHUNK_SIZE;
    slice.data = ar_alloc(slice.capacity * slice.element_size);
    memcpy(slice.data,
           (char *)arr->data + (arr->offset + start) * arr->element_size,
           slice.size * arr->element_size);

    slice.offset = 0;
    RWLOCK_CREATE(&slice.lock);
    slice.gc_objects = arr->gc_objects;
    if (slice.gc_objects) // the copy holds its own references
      for (size_t i = 0; i < slice.size; i++)
        gc_incref(((struct ArgonObject **)slice.data)[i]);
  });

  return slice;
}
