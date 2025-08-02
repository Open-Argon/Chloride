/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AROBJECT_H
#define AROBJECT_H

#include "runtime/internals/dynamic_array_armem/darray_armem.h"
#include "runtime/internals/hashmap/hashmap.h"
#include <gmp.h>

typedef struct ArgonObject ArgonObject; // forward declaration

typedef enum ArgonType {
  TYPE_NULL,
  TYPE_BOOL,
  TYPE_NUMBER,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_NATIVE_FUNCTION,
  TYPE_OBJECT,
} ArgonType;

struct string_struct {
  char *data;
  size_t length;
};

typedef struct Stack {
  struct hashmap_GC *scope;
  struct Stack *prev;
} Stack;

struct argon_function_struct {
  darray_armem bytecode;
  Stack stack;
  size_t number_of_parameters;
  char **parameters;
};



// full definition of ArgonObject (no typedef again!)
struct ArgonObject {
  ArgonType type;
  struct hashmap_GC *dict;
  union {
    mpq_t as_number;
    bool as_bool;
    struct string_struct as_str;
    void *native_fn;
    struct argon_function_struct argon_fn;
  } value;
};

#endif // AROBJECT_H