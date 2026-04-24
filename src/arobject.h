/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AROBJECT_H
#define AROBJECT_H

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
  X(from_string)                                                               \
  X(to_string)                                                                 \
  X(__getattribute__)                                                          \
  X(__negation__)                                                              \
  X(__setattr__)                                                               \
  X(__delattr__)                                                               \
  X(__getitem__)                                                               \
  X(__setitem__)                                                               \
  X(__delitem__)                                                               \
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
  X(join)

typedef enum {
#define X(name) name,
  BUILT_IN_FIELDS(X)
#undef X
      field__address,
  BUILT_IN_FIELDS_COUNT
} built_in_fields;

typedef struct RuntimeState RuntimeState;

typedef struct ArgonObject ArgonObject; // forward declaration

typedef struct {
  ArgonObject *ptr;
} ArErr;

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

struct array {
  ArgonObject **items;
  size_t size;
};

struct ArgonNativeAPI {
  void (*register_ArgonObject)(hashmap_GC *reg, char *name, ArgonObject *obj);
  ArgonObject *(*create_argon_native_function)(char *name, native_fn);
  ArgonObject *(*call)(ArgonObject *original_object, size_t argc,
                       ArgonObject **argv, ArErr *err, RuntimeState *state);
  ArgonObject *(*throw_argon_error)(ArErr *err, ArgonObject *type,
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

  int (*register_thread)();
  int (*unregister_thread)();

  ArgonObject *(*create_err_object)();
  ArErr *(*err_object_to_err)(ArgonObject *, ArErr *);
  RuntimeState *(*new_state)(ArgonObject **registers);
  void (*set_err)(ArgonObject *object, ArErr *err);
  void *(*malloc)(size_t);
  void (*free)(void *);

  struct array (*argon_to_array)(ArgonObject *, ArErr *);
  int (*argon_get_ArgonType)(ArgonObject *);
  bool (*argon_is_i64)(ArgonObject *);

  ArgonObject *BaseException;
  ArgonObject *Exception;
  ArgonObject *RuntimeError;
  ArgonObject *SyntaxError;
  ArgonObject *ConversionError;
  ArgonObject *MathsError;
  ArgonObject *ZeroDivisionError;
  ArgonObject *NameError;
  ArgonObject *TypeError;
  ArgonObject *InternalError;
  ArgonObject *IndexError;
  ArgonObject *AttributeError;
  ArgonObject *PathError;
  ArgonObject *FileError;
  ArgonObject *ImportError;
  ArgonObject *SignalException;
  ArgonObject *SignalKeyboardInterrupt;
  ArgonObject *SignalStopIteration;

  ArgonObject *(*throw_argon_signal)(ArErr *, ArgonObject *);
};

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
  struct hashmap_GC *dict;
  size_t built_in_slot_length;
  struct built_in_slot built_in_slot[BUILT_IN_ARRAY_COUNT];
  ArgonType type;
  bool as_bool;
  // pthread_rwlock_t lock;
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