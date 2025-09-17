/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AROBJECT_H
#define AROBJECT_H

#include "dynamic_array/darray.h"
#include "runtime/internals/dynamic_array_armem/darray_armem.h"
#include "runtime/internals/hashmap/hashmap.h"
#include <gmp.h>

typedef enum {
  __base__,
  __class__,
  __name__,
  __binding__,
  __function__,

  BUILT_IN_ARRAY_COUNT,

  __add__,
  __string__,
  __subtract__,
  __multiply__,
  __division__,
  __new__,
  __init__,
  __boolean__,
  __get_attr__,
  field__address,
  __call__,
  __number__,
  field_log,
  field_length,
  __getattribute__,
  __setattr__,
  __hash__,
  __repr__,

  BUILT_IN_FIELDS_COUNT,
} built_in_fields;

typedef struct ArErr ArErr;
typedef struct RuntimeState RuntimeState;

typedef struct ArgonObject ArgonObject; // forward declaration

typedef ArgonObject *(*native_fn)(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state);

typedef enum ArgonType {
  TYPE_NULL,
  TYPE_BOOL,
  TYPE_NUMBER,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_NATIVE_FUNCTION,
  TYPE_METHOD,
  TYPE_DICTIONARY,
  TYPE_OBJECT,
} ArgonType;

typedef struct {
  void *data;
  size_t capacity;
  size_t size;
  struct hashmap *hashmap;
} ConstantArena;

typedef struct {
  uint8_t registerCount;
  uint8_t registerAssignment;
  DArray *return_jumps;
  DArray bytecode;
  ConstantArena constants;
  char *path;
} Translated;

struct string_struct {
  uint64_t prehash;
  uint64_t hash;
  char *data;
  size_t length;
  bool hash_computed;
};

typedef struct Stack {
  uint64_t fake_new_scopes;
  struct hashmap_GC *scope;
  struct Stack *prev;
} Stack;

struct argon_function_struct {
  Translated translated;
  uint8_t *bytecode;
  size_t bytecode_length;
  Stack *stack;
  size_t number_of_parameters;
  struct string_struct *parameters;
  uint64_t line;
  uint64_t column;
};

struct built_in_slot {
  built_in_fields field;
  ArgonObject *value;
};

struct as_number {
  union {
    mpq_t *mpq;
    int64_t i64;
  } n;
  bool is_int64;
};

// full definition of ArgonObject (no typedef again!)
struct ArgonObject {
  struct hashmap_GC *dict;
  size_t built_in_slot_length;
  struct built_in_slot built_in_slot[BUILT_IN_ARRAY_COUNT];
  union {
    struct as_number *as_number;
    struct hashmap_GC *as_hashmap;
    struct string_struct *as_str;
    native_fn native_fn;
    struct argon_function_struct *argon_fn;
  } value;
  ArgonType type;
  bool as_bool;
};

#endif // AROBJECT_H