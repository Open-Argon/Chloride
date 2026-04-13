/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "array.h"
#include "../../../../include/ArgonTypes.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../../internals/dynamic_array_armem/darray_armem.h"
#include "../exceptions/exceptions.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../slice/slice.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARRAY_TYPE;
ArgonObject *ARRAY_ITERATOR_TYPE;
ArgonObject *ARGON_ARRAY_CREATE;

ArgonObject *ARRAY_CREATE(size_t argc, ArgonObject **argv, ArErr *err,
                          RuntimeState *state, ArgonNativeAPI *api) {
  (void)err;
  (void)state;
  (void)api;
  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;

  object->value.as_array = darray_armem_create();

  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), argc);

  memcpy(object->value.as_array->data, argv, argc * sizeof(ArgonObject *));

  return object;
}

ArgonObject *ARGON_ARRAY___new__(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError, "__new__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *get_dictionary = get_builtin_field_for_class(
      get_builtin_field(argv[1], __class__), __array__, argv[1]);
  if (!get_dictionary)
    return api->throw_argon_error(err, RuntimeError,
                                  "Object doesn't have __array__ method");
  ArgonObject *object = argon_call(get_dictionary, 0, NULL, err, state);
  if (object->type != TYPE_ARRAY)
    return api->throw_argon_error(
        err, RuntimeError, "Objects __array__ method didn't return an array");
  return object;
}

ArgonObject *ARGON_ARRAY___string__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;

  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (!is_being_repr)
    is_being_repr = createHashmap();

  if (hashmap_lookup(is_being_repr, (uint64_t)argv[0]))
    return new_string_object_null_terminated("[...]");

  hashmap_insert(is_being_repr, (uint64_t)argv[0], NULL, (void *)true, 0);

  darray_armem *array = argv[0]->value.as_array;

  size_t capacity = 32;
  char *string = checked_malloc(capacity);
  size_t string_length = 1;
  string[0] = '[';

  for (size_t i = 0; i < array->size; i++) {
    ArgonObject *item = *(ArgonObject **)darray_armem_get(array, i);
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
          argon_call(string_convert_method, 0, NULL, err, state);
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
  string[string_length] = ']';
  string_length += 1;

  ArgonObject *result = new_string_object(string, string_length, 0);

  free(string);
  hashmap_remove(is_being_repr, (uint64_t)argv[0]);
  if (!is_being_repr->count)
    hashmap_free(is_being_repr, NULL);
  return result;
}

ArgonObject *ARGON_ARRAY_append(size_t argc, ArgonObject **argv, ArErr *err,
                                RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err = create_err(RuntimeError, "append expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  darray_armem_insert(argv[0]->value.as_array, UINT64_MAX, &argv[1]);
  return ARGON_NULL;
}

ArgonObject *ARGON_ARRAY_of_size(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  (void)api;
  if (argc != 1) {
    *err = create_err(RuntimeError, "of_size expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  int64_t size = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;
  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;

  object->value.as_array = darray_armem_create();

  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), size);

  for (int64_t i = 0; i < size; i++) {
    ((ArgonObject **)object->value.as_array->data)[i] = ARGON_NULL;
  }

  return object;
}

ArgonObject *ARGON_ARRAY___array__(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "__array__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return argv[0];
}

ArgonObject *ARGON_ARRAY___contains__(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__contains__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object_class = get_builtin_field(argv[1], __class__);
  ArgonObject *object__equal__ =
      get_builtin_field_for_class(object_class, __equal__, argv[1]);
  if (!object__equal__) {
    ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
    *err = create_err(RuntimeError,
                      "Object of type '%.*s' is missing __equal__ method",
                      (int)cls___name__->value.as_str->length,
                      cls___name__->value.as_str->data);
    return ARGON_NULL;
  }
  darray_armem *arr = argv[0]->value.as_array;
  for (size_t i = 0; i < arr->size; i++) {
    ArgonObject *result =
        argon_call(object__equal__, 1,
                   (ArgonObject *[]){*(ArgonObject **)darray_armem_get(arr, i)},
                   err, state);
    if (api->is_error(err))
      return ARGON_NULL;
    if (result == ARGON_TRUE)
      return result;
  }
  return ARGON_FALSE;
}

ArgonObject *ARGON_ARRAY_get_length(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "get_length expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0]->value.as_array->size);
}

ArgonObject *ARGON_ARRAY_set_length(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
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
}

ArgonObject *ARGON_ARRAY_insert(size_t argc, ArgonObject **argv, ArErr *err,
                                RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  if (argc != 3) {
    *err = create_err(RuntimeError, "insert expects 3 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  size_t pos = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  darray_armem_insert(argv[0]->value.as_array, pos, &argv[2]);
  return ARGON_NULL;
}

ArgonObject *ARGON_ARRAY_pop(size_t argc, ArgonObject **argv, ArErr *err,
                             RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  if (argc > 2 || argc < 1) {
    *err = create_err(RuntimeError,
                      "pop expects 1 or 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  size_t pos = UINT64_MAX;
  if (argc == 2)
    pos = api->argon_to_i64(argv[1], err);
  darray_armem *arr = argv[0]->value.as_array;
  if (api->is_error(err))
    return ARGON_NULL;
  ArgonObject *out;
  if (!darray_armem_pop(arr, pos, &out))
    return api->throw_argon_error(err, IndexError, "pop from empty array");
  return out;
}

ArgonObject *ARGON_ARRAY___getitem__(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__getitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  darray_armem *arr = argv[0]->value.as_array;
  if (argv[1]->type == TYPE_SLICE) { // slice
    ArgonObject *indices = ARGON_SLICE_TYPE_indices(
        2,
        (ArgonObject *[]){argv[1], new_number_object_from_int64(
                                       arr->size)},
        err, state, api);
    if (api->is_error(err))
      return ARGON_NULL;
    int64_t start = api->argon_to_i64(indices->value.as_tuple.data[0],err);
    if (api->is_error(err))
      return ARGON_NULL;
    int64_t stop = api->argon_to_i64(indices->value.as_tuple.data[1],err);
    if (api->is_error(err))
      return ARGON_NULL;
    int64_t step = api->argon_to_i64(indices->value.as_tuple.data[2],err);
    if (api->is_error(err))
      return ARGON_NULL;

    ArgonObject *slice = new_instance(ARRAY_TYPE, sizeof(darray_armem));
    slice->type = TYPE_ARRAY;

    slice->value.as_array = darray_armem_create();

    int64_t size = 0;
    if (step > 0 && stop > start)
        size = (stop - start + step - 1) / step;
    else if (step < 0 && stop < start)
        size = (start - stop - step - 1) / (-step);

    darray_armem_init(slice->value.as_array, sizeof(ArgonObject *), size);

    for (int64_t i = 0; i < size; i++) {
        int64_t src_index = start + i * step;  // source index follows step
        ((ArgonObject**)slice->value.as_array->data)[i] =
            *(ArgonObject **)darray_armem_get(arr, src_index);
    }

    return slice;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (index < 0)
    index += arr->size;
  if (index >= (int64_t)arr->size || index < 0) {
    return api->throw_argon_error(err, IndexError, "index out of range");
  }
  return *(ArgonObject **)darray_armem_get(arr, index);
}

ArgonObject *ARGON_ARRAY___setitem__(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)state;
  if (argc != 3) {
    *err = create_err(RuntimeError,
                      "__setitem__ expects 3 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  darray_armem *arr = argv[0]->value.as_array;
  if (index < 0)
    index += arr->size;
  if (index >= (int64_t)arr->size || index < 0) {
    return api->throw_argon_error(err, IndexError, "index out of range");
  }
  ArgonObject **ptr = darray_armem_get(arr, index);
  *ptr = argv[2];
  return *ptr;
}

ArgonObject *ARGON_ARRAY___iter__(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *iterator =
      new_instance(ARRAY_ITERATOR_TYPE, sizeof(struct as_array_iterator));
  iterator->type = TYPE_ARRAY_ITERATOR;
  iterator->value.as_array_iterator =
      (struct as_array_iterator *)((char *)iterator + sizeof(ArgonObject));
  iterator->value.as_array_iterator->current = 0;
  iterator->value.as_array_iterator->array = self->value.as_array;
  return iterator;
}

ArgonObject *ARGON_ARRAY_ITERATOR___next__(size_t argc, ArgonObject **argv,
                                           ArErr *err, RuntimeState *state,
                                           ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__next__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_array_iterator *array_iterator = self->value.as_array_iterator;

  if (array_iterator->current >= array_iterator->array->size) {
    err->ptr = StopIteration_instance;
    return ARGON_NULL;
  }

  ArgonObject *value = *(ArgonObject **)darray_armem_get(
      array_iterator->array, array_iterator->current++);

  return value;
}

void init_array_type() {
  ARRAY_TYPE = new_class();
  add_builtin_field(ARRAY_TYPE, __name__,
                    new_string_object_null_terminated("array"));
  add_builtin_field(
      ARRAY_TYPE, __new__,
      create_argon_native_function("__new__", ARGON_ARRAY___new__));
  add_builtin_field(
      ARRAY_TYPE, __string__,
      create_argon_native_function("__string__", ARGON_ARRAY___string__));
  add_builtin_field(ARRAY_TYPE, append,
                    create_argon_native_function("append", ARGON_ARRAY_append));
  add_builtin_field(ARRAY_TYPE, insert,
                    create_argon_native_function("insert", ARGON_ARRAY_insert));
  add_builtin_field(ARRAY_TYPE, pop,
                    create_argon_native_function("pop", ARGON_ARRAY_pop));
  add_builtin_field(
      ARRAY_TYPE, get_length,
      create_argon_native_function("get_length", ARGON_ARRAY_get_length));
  add_builtin_field(
      ARRAY_TYPE, set_length,
      create_argon_native_function("set_length", ARGON_ARRAY_set_length));
  add_builtin_field(
      ARRAY_TYPE, __getitem__,
      create_argon_native_function("__getitem__", ARGON_ARRAY___getitem__));
  add_builtin_field(
      ARRAY_TYPE, __setitem__,
      create_argon_native_function("__setitem__", ARGON_ARRAY___setitem__));
  add_builtin_field(
      ARRAY_TYPE, __contains__,
      create_argon_native_function("__contains__", ARGON_ARRAY___contains__));
  add_builtin_field(
      ARRAY_TYPE, __iter__,
      create_argon_native_function("__iter__", ARGON_ARRAY___iter__));
  add_builtin_field(
      ARRAY_TYPE, of_size,
      create_argon_native_function("of_size", ARGON_ARRAY_of_size));
  add_builtin_field(
      ARRAY_TYPE, __array__,
      create_argon_native_function("__array__", ARGON_ARRAY___array__));

  ARRAY_ITERATOR_TYPE = new_class();
  add_builtin_field(
      ARRAY_ITERATOR_TYPE, __next__,
      create_argon_native_function("__next__", ARGON_ARRAY_ITERATOR___next__));

  ARGON_ARRAY_CREATE =
      create_argon_native_function("ARRAY_CREATE", ARRAY_CREATE);
}