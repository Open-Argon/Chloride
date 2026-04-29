/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tuple.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../exceptions/exceptions.h"

#include "../literals/literals.h"
#include "../number/number.h"
#include "../slice/slice.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARGON_TUPLE_TYPE;
ArgonObject *ARGON_TUPLE_ITERATOR_TYPE;
ArgonObject *ARGON_TUPLE_CREATE;

ARGON_FUNCTION(TUPLE_CREATE, {
  (void)err;
  (void)state;
  (void)api;
  ArgonObject *object =
      new_instance(ARGON_TUPLE_TYPE, argc * sizeof(ArgonObject *));
  object->type = TYPE_TUPLE;
  object->value.as_tuple.size = argc;
  memcpy(object->value.as_tuple.data, argv, argc * sizeof(ArgonObject *));

  return object;
})

ARGON_METHOD(ARGON_TUPLE_TYPE, __new__, {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(RuntimeError,
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj =
      new_instance(argv[0], (argc - 1) * sizeof(ArgonObject *));
  new_obj->type = TYPE_TUPLE;
  new_obj->value.as_tuple.size = (argc - 1);
  memcpy(new_obj->value.as_tuple.data, argv + 1,
         (argc - 1) * sizeof(ArgonObject *));
  return new_obj;
})

ARGON_METHOD(ARGON_TUPLE_TYPE, of, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err =
        create_err(RuntimeError, "of expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  ArgonObject *iter_method = get_builtin_field_for_class(
      get_builtin_field(argv[0], __class__), __iter__, argv[0]);
  if (!iter_method)
    return api->throw_argon_error(err, RuntimeError,
                                  "Object doesn't have __iter__ method");
  ArgonObject *iter_obj = argon_call(iter_method, 0, NULL, NULL, err, state);

  ArgonObject *next_method = get_builtin_field_for_class(
      get_builtin_field(iter_obj, __class__), __next__, iter_obj);
  if (!next_method)
    return api->throw_argon_error(
        err, RuntimeError, "Iterator object doesn't have __next__ method");

  size_t items_index = 0;
  size_t items_size = 8;
  ArgonObject **items = ar_alloc(items_size * sizeof(ArgonObject *));

  while (true) {
    ArgonObject *item = argon_call(next_method, 0, NULL, NULL, err, state);
    if (err->ptr == StopIteration_instance) {
      err->ptr = ARGON_NULL;
      break;
    } else if (api->is_error(err)) {
      return ARGON_NULL;
    }
    if (items_size <= items_index) {
      items_size *= 2;
      items = ar_realloc(items, items_size * sizeof(ArgonObject *));
    }
    items[items_index++] = item;
  }

  ArgonObject *object =
      new_instance(ARGON_TUPLE_TYPE, items_index * sizeof(ArgonObject *));
  object->type = TYPE_TUPLE;
  object->value.as_tuple.size = items_index;
  memcpy(object->value.as_tuple.data, items,
         items_index * sizeof(ArgonObject *));
  return object;
})

ARGON_METHOD(ARGON_TUPLE_TYPE, __string__, {
  (void)api;

  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (!is_being_repr)
    is_being_repr = createHashmap();

  if (hashmap_lookup(is_being_repr, (uint64_t)argv[0]))
    return new_string_object_null_terminated("tuple(...)");

  hashmap_insert(is_being_repr, (uint64_t)argv[0], NULL, (void *)true, 0);

  struct tuple_struct *tuple = &argv[0]->value.as_tuple;

  size_t capacity = 32;
  char *string = checked_malloc(capacity);
  char beginning[] = "tuple(";
  size_t string_length = sizeof(beginning) - 1;
  memcpy(string, beginning, string_length);

  for (size_t i = 0; i < tuple->size; i++) {
    ArgonObject *item = tuple->data[i];
    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(item, __class__), __repr__, item);

    if (i) {
      char *string_obj = ", ";
      size_t length = strlen(string_obj);
      bool resized = false;
      while (capacity < string_length + length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, NULL, err, state);
      bool resized = false;
      while (capacity < string_length + string_object->value.as_str->length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_object->value.as_str->data,
             string_object->value.as_str->length);
      string_length += string_object->value.as_str->length;
    } else {
      char *string_obj = "<object>";
      size_t length = strlen(string_obj);
      bool resized = false;
      while (capacity < string_length + length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }
  }
  if (capacity < string_length + 1) {
    capacity += 1;
    string = realloc(string, capacity);
  }
  string[string_length] = ')';
  string_length += 1;

  ArgonObject *result = new_string_object(string, string_length, 0);

  free(string);

  hashmap_remove(is_being_repr, (uint64_t)argv[0]);
  if (!is_being_repr->count)
    hashmap_free(is_being_repr, NULL);
  return result;
})

ARGON_METHOD(ARGON_TUPLE_TYPE, get_length, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "get_length expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0]->value.as_tuple.size);
})

ARGON_METHOD(ARGON_TUPLE_TYPE, set_length, {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "set_length expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  *err = create_err(RuntimeError, "attribute 'length' is immutable");
  return ARGON_NULL;
})

ARGON_METHOD(ARGON_TUPLE_TYPE, __getitem__, {
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__getitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  struct tuple_struct *tuple = &argv[0]->value.as_tuple;

  if (argv[1]->type == TYPE_SLICE) { // slice

    SliceIndices indices;
    if (slice_indices(argv[1], tuple->size, &indices, err, api) != 0)
      return ARGON_NULL;

    int64_t size = 0;
    if (indices.step > 0 && indices.stop > indices.start)
      size = (indices.stop - indices.start + indices.step - 1) / indices.step;
    else if (indices.step < 0 && indices.stop < indices.start)
      size =
          (indices.start - indices.stop - indices.step - 1) / (-indices.step);

    ArgonObject *slice =
        new_instance(ARGON_TUPLE_TYPE, size * sizeof(ArgonObject *));
    slice->type = TYPE_TUPLE;
    slice->value.as_tuple.size = size;

    for (int64_t i = 0; i < size; i++) {
      int64_t src_index =
          indices.start + i * indices.step; // source index follows step
      slice->value.as_tuple.data[i] = tuple->data[src_index];
    }

    return slice;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (index < 0)
    index += tuple->size;
  if (index >= (int64_t)tuple->size || index < 0) {
    return api->throw_argon_error(err, IndexError, "index out of range");
  }
  return tuple->data[index];
})

ARGON_METHOD(ARGON_TUPLE_TYPE, __iter__, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *iterator =
      new_instance(ARGON_TUPLE_ITERATOR_TYPE, sizeof(struct as_tuple_iterator));
  iterator->type = TYPE_TUPLE_ITERATOR;
  iterator->value.as_tuple_iterator =
      (struct as_tuple_iterator *)((char *)iterator + sizeof(ArgonObject));
  iterator->value.as_tuple_iterator->current = 0;
  iterator->value.as_tuple_iterator->tuple = &self->value.as_tuple;
  return iterator;
})

ARGON_METHOD(ARGON_TUPLE_ITERATOR_TYPE, __next__, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__next__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_tuple_iterator *tuple_iterator = self->value.as_tuple_iterator;

  if (tuple_iterator->current >= tuple_iterator->tuple->size) {
    err->ptr = StopIteration_instance;
    return ARGON_NULL;
  }

  ArgonObject *value = tuple_iterator->tuple->data[tuple_iterator->current++];

  return value;
})

void init_tuple_type() {
  ARGON_TUPLE_TYPE = new_class();
  add_builtin_field(ARGON_TUPLE_TYPE, __name__,
                    new_string_object_null_terminated("tuple"));
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, __new__)
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, __string__)
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, get_length)
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, set_length)
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, __getitem__)
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, __iter__)
  MOUNT_ARGON_METHOD(ARGON_TUPLE_TYPE, of)

  ARGON_TUPLE_ITERATOR_TYPE = new_class();
  MOUNT_ARGON_METHOD(ARGON_TUPLE_ITERATOR_TYPE, __next__);

  ARGON_TUPLE_CREATE =
      create_argon_native_function("TUPLE_CREATE", TUPLE_CREATE);
}