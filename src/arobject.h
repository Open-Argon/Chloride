/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AROBJECT_H
#define AROBJECT_H

#include "../include/Argon.h"
#include "../include/ArgonTypes.h"
#include "dynamic_array/darray.h"
#include "runtime/internals/dynamic_array_armem/darray_armem.h"
#include "runtime/internals/hashmap/hashmap.h"
#include <gmp.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define BUILT_IN_FIELDS(X)                                                     \
  X(__base__)                                                                  \
  X(__class__)                                                                 \
  X(__name__)                                                                  \
  X(BUILT_IN_ARRAY_COUNT)                                                      \
  X(__binding__)                                                               \
  X(__function__)                                                              \
  X(__add__)                                                                   \
  X(__string__)                                                                \
  X(__subtract__)                                                              \
  X(__multiply__)                                                              \
  X(__exponent__)                                                              \
  X(__division__)                                                              \
  X(__floor_division__)                                                        \
  X(__modulo__)                                                                \
  X(__equal__)                                                                 \
  X(__not_equal__)                                                             \
  X(__less_than__)                                                             \
  X(__less_than_equal__)                                                       \
  X(__greater_than__)                                                          \
  X(__greater_than_equal__)                                                    \
  X(__new__)                                                                   \
  X(__init__)                                                                  \
  X(__boolean__)                                                               \
  X(__getattr__)                                                               \
  X(__call__)                                                                  \
  X(__number__)                                                                \
  X(get_length)                                                                \
  X(set_length)                                                                \
  X(of_size)                                                                   \
  X(extend)                                                                    \
  X(from_string)                                                               \
  X(to_string)                                                                 \
  X(partition)                                                                 \
  X(__getattribute__)                                                          \
  X(__negation__)                                                              \
  X(__setattr__)                                                               \
  X(__delattr__)                                                               \
  X(__getitem__)                                                               \
  X(__setitem__)                                                               \
  X(__delitem__)                                                               \
  X(value_get)                                                                 \
  X(__hash__)                                                                  \
  X(__repr__)                                                                  \
  X(append)                                                                    \
  X(insert)                                                                    \
  X(pop)                                                                       \
  X(__contains__)                                                              \
  X(__iter__)                                                                  \
  X(of)                                                                        \
  X(__next__)                                                                  \
  X(__template__)                                                              \
  X(__dictionary__)                                                            \
  X(message)                                                                   \
  X(stack_trace)                                                               \
  X(indices)                                                                   \
  X(start)                                                                     \
  X(step)                                                                      \
  X(stop)                                                                      \
  X(split)                                                                     \
  X(join)                                                                      \
  X(chr)                                                                       \
  X(ord)                                                                       \
  X(upper)                                                                     \
  X(lower)                                                                     \
  X(title)                                                                     \
  X(replace)                                                                   \
  X(strip)                                                                     \
  X(index_of)                                                                  \
  X(map)                                                                       \
  X(filter)                                                                    \
  X(sort)

typedef enum {
#define X(name) name,
  BUILT_IN_FIELDS(X)
#undef X
      field__address,
  BUILT_IN_FIELDS_COUNT
} built_in_fields;

typedef struct RuntimeState RuntimeState;

typedef struct ArgonObject ArgonObject; // forward declaration

struct ArErr {
  ArgonObject *ptr;
};

#undef ArgonError

typedef struct ArgonNativeAPI ArgonNativeAPI;

typedef struct hashmap_GC hashmap_GC;

typedef struct {
  void *data;
  size_t capacity;
  size_t size;
  struct hashmap *hashmap;
} ConstantArena;

struct continue_jump {
  int64_t pos;
  uint64_t exception_handler_depth;
  uint64_t scope_depth;
};

struct break_or_return_jump {
  DArray *positions;
  uint64_t exception_handler_depth;
  uint64_t scope_depth;
};

typedef struct {
  uint8_t registerCount;
  uint8_t registerAssignment;
  uint64_t scope_depth;
  uint64_t exception_handler_depth;
  struct continue_jump continue_jump;
  struct break_or_return_jump return_jump;
  struct break_or_return_jump break_jump;
  DArray bytecode;
  ConstantArena constants;
  char *path;
} Translated;

struct string_struct {
  uint64_t hash;
  char *data;
  size_t length;
};

typedef struct Stack {
  uint64_t fake_new_scopes;
  struct hashmap_GC *scope;
  struct Stack *prev;
} Stack;

struct default_value {
  struct string_struct key;
  ArgonObject *value;
};

struct argon_function_struct {
  Translated translated;
  uint8_t *bytecode;
  size_t bytecode_length;
  Stack *stack;
  size_t number_of_parameters;
  struct string_struct *parameters;
  size_t number_of_default_parameters;
  struct default_value *default_parameters;
  struct string_struct vargs;
  struct string_struct kwargs;
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

struct as_range_iterator {
  bool is_int64;
  bool inclusive;
  union {
    ArgonObject *obj;
    int64_t i64;
  } current;
  union {
    ArgonObject *obj;
    int64_t i64;
  } stop;
  union {
    ArgonObject *obj;
    int64_t i64;
  } step;
};

struct as_array_iterator {
  size_t current;
  darray_armem *array;
};

struct as_string_iterator {
  size_t current;
  char *data;
  size_t length;
};

struct as_dictionary_iterator {
  size_t current;
  size_t size;
  struct node_GC **array;
};

struct tuple_struct {
  size_t size;
  ArgonObject *data[];
};

struct as_tuple_iterator {
  size_t current;
  struct tuple_struct *tuple;
};

// full definition of ArgonObject (no typedef again!)
struct ArgonObject {
  ArgonType type;
  bool as_bool;
  uint8_t built_in_slot_length;
  struct built_in_slot built_in_slot[BUILT_IN_ARRAY_COUNT];
  struct hashmap_GC *dict;
  union {
    ArErr err;
    struct as_number *as_number;
    struct hashmap_GC *as_hashmap;
    struct as_range_iterator *as_range_iterator;
    struct as_array_iterator *as_array_iterator;
    struct as_string_iterator *as_string_iterator;
    struct as_tuple_iterator *as_tuple_iterator;
    struct as_dictionary_iterator *as_dictionary_iterator;
    struct string_struct *as_str;
    struct tuple_struct as_tuple;
    struct buffer *as_buffer;
    darray_armem *as_array;
    native_fn native_fn;
    struct argon_function_struct *argon_fn;
  } value;
};

bool is_error(ArErr *err);

#endif // AROBJECT_H