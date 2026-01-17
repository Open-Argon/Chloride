/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AROBJECT_H
#define AROBJECT_H

#include "dynamic_array/darray.h"
#include "runtime/internals/hashmap/hashmap.h"
#include <gmp.h>
#include <stddef.h>

typedef enum {
  __base__,
  __class__,
  __name__,

  BUILT_IN_ARRAY_COUNT,

  __binding__,
  __function__,
  __add__,
  __string__,
  __subtract__,
  __multiply__,
  __exponent__,
  __division__,
  __floor_division__,
  __modulo__,
  __equal__,
  __not_equal__,
  __less_than__,
  __less_than_equal__,
  __greater_than__,
  __greater_than_equal__,
  __new__,
  __init__,
  __boolean__,
  __getattr__,
  field__address,
  __call__,
  __number__,
  get_length,
  set_length,
  of_size,
  from_string,
  to_string,
  __getattribute__,
  __negation__,
  __setattr__,
  __getitem__,
  __setitem__,
  __hash__,
  __repr__,

  BUILT_IN_FIELDS_COUNT,
} built_in_fields;

typedef struct ArErr ArErr;
typedef struct RuntimeState RuntimeState;

typedef struct ArgonObject ArgonObject; // forward declaration

typedef struct ArgonNativeAPI ArgonNativeAPI;

typedef struct hashmap_GC hashmap_GC;

typedef ArgonObject *(*native_fn)(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state, ArgonNativeAPI *api);

struct rational {
  int64_t n;
  int64_t d;
};

struct string {
  char *data;
  size_t length;
};

struct buffer {
  void *data;
  size_t size;
};

struct ArgonNativeAPI {
  void (*register_ArgonObject)(hashmap_GC *reg, char *name, ArgonObject *obj);
  ArgonObject *(*create_argon_native_function)(char *name, native_fn);
  ArgonObject *(*throw_argon_error)(ArErr *err, const char *type,
                                    const char *fmt, ...);
  bool (*is_error)(ArErr *err);
  bool (*fix_to_arg_size)(size_t limit, size_t argc, ArErr *err);

  // numbers
  ArgonObject *(*i64_to_argon)(int64_t);
  ArgonObject *(*double_to_argon)(double);
  ArgonObject *(*rational_to_argon)(struct rational);
  int64_t (*argon_to_i64)(ArgonObject *, ArErr *);
  double (*argon_to_double)(ArgonObject *, ArErr *);
  struct rational (*argon_to_rational)(ArgonObject *, ArErr *);

  // strings
  ArgonObject *(*string_to_argon)(struct string);
  struct string (*argon_to_string)(ArgonObject *, ArErr *);

  // buffers
  ArgonObject *(*create_argon_buffer)(size_t size);
  void (*resize_argon_buffer)(ArgonObject *obj, ArErr *err, size_t new_size);
  struct buffer (*argon_buffer_to_buffer)(ArgonObject *obj, ArErr *err);

  // literals
  ArgonObject *ARGON_NULL;
  ArgonObject *ARGON_TRUE;
  ArgonObject *ARGON_FALSE;
};

typedef enum ArgonType {
  TYPE_NULL,
  TYPE_BOOL,
  TYPE_NUMBER,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_NATIVE_FUNCTION,
  TYPE_METHOD,
  TYPE_DICTIONARY,
  TYPE_BUFFER,
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
    struct buffer *as_buffer;
    native_fn native_fn;
    struct argon_function_struct *argon_fn;
  } value;
  ArgonType type;
  bool as_bool;
};

#endif // AROBJECT_H