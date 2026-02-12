/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "array.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../../internals/dynamic_array_armem/darray_armem.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARRAY_TYPE;
ArgonObject *ARGON_ARRAY_CREATE;

ArgonObject *ARRAY_CREATE(size_t argc, ArgonObject **argv, ArErr *err,
                          RuntimeState *state, ArgonNativeAPI *api) {
  (void)err;
  (void)state;
  (void)api;
  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;

  object->value.as_array =
      (darray_armem *)((char *)object + sizeof(ArgonObject));

  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), argc);

  memcpy(object->value.as_array->data, argv, argc * sizeof(ArgonObject *));

  return object;
}

ArgonObject *ARGON_ARRAY___string__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;

  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

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

  ArgonObject *result = new_string_object(string, string_length, 0, 0);

  free(string);

  return result;
}

ArgonObject *ARGON_ARRAY_append(size_t argc, ArgonObject **argv, ArErr *err,
                                RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "append expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  darray_armem_insert(argv[0]->value.as_array, UINT64_MAX, &argv[1]);
  return ARGON_NULL;
}

ArgonObject *ARGON_ARRAY___contains__(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__contains__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object_class = get_builtin_field(argv[1], __class__);
  ArgonObject *object__equal__ =
      get_builtin_field_for_class(object_class, __equal__, argv[1]);
  if (!object__equal__) {
    ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                     "Object of type '%.*s' is missing __equal__ method",
                     (int)cls___name__->value.as_str->length,
                     cls___name__->value.as_str->data);
    return ARGON_NULL;
  }
  darray_armem* arr = argv[0]->value.as_array;
  for (size_t i = 0;i<arr->size;i++) {
    ArgonObject*result = argon_call(object__equal__, 1, (ArgonObject*[]){darray_armem_get(arr, i)}, err, state);
    if (api->is_error(err)) return ARGON_NULL;
    printf("%d\n", result==ARGON_FALSE);
    if (result == ARGON_TRUE) return result;
  }
  return ARGON_FALSE;
}

ArgonObject *ARGON_ARRAY_get_length(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
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
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "set_length expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  *err = create_err(0, 0, 0, "", "Runtime Error",
                    "attribute 'length' is immutable");
  return ARGON_NULL;
}

ArgonObject *ARGON_ARRAY_insert(size_t argc, ArgonObject **argv, ArErr *err,
                                RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "insert expects 3 arguments, got %" PRIu64, argc);
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
    *err = create_err(0, 0, 0, "", "Runtime Error",
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
    return api->throw_argon_error(err, "Index Error", "pop from empty array");
  return out;
}

ArgonObject *ARGON_ARRAY___getitem__(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__getitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  darray_armem *arr = argv[0]->value.as_array;
  if (index < 0)
    index += arr->size;
  if (index >= (int64_t)arr->size || index < 0) {
    return api->throw_argon_error(err, "Index Error", "index out of range");
  }
  return *(ArgonObject **)darray_armem_get(arr, index);
}

ArgonObject *ARGON_ARRAY___setitem__(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
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
    return api->throw_argon_error(err, "Index Error", "index out of range");
  }
  ArgonObject **ptr = darray_armem_get(arr, index);
  *ptr = argv[2];
  return *ptr;
}

void init_array_type() {
  ARRAY_TYPE = new_class();
  add_builtin_field(ARRAY_TYPE, __name__,
                    new_string_object_null_terminated("array"));
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

  ARGON_ARRAY_CREATE =
      create_argon_native_function("ARRAY_CREATE", ARRAY_CREATE);
}