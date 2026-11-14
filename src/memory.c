/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Backend-independent memory helpers. The GC backends live in src/gc/.

#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *checked_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "fatal error: failed to allocate %zu bytes\n", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *checked_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "fatal error: failed to allocate %zu bytes\n", size);
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

char *ar_strdup(const char *str) {
  size_t len = strlen(str) + 1;
  char *copy = ar_alloc_atomic(len);
  memcpy(copy, str, len);
  return copy;
}
