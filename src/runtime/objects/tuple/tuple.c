/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tuple.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARGON_TUPLE_TYPE;
ArgonObject *ARGON_TUPLE_CREATE;

ArgonObject *TUPLE_CREATE(size_t argc, ArgonObject **argv, ArErr *err,
                          RuntimeState *state, ArgonNativeAPI *api) {
  (void)err;
  (void)state;
  (void)api;
  ArgonObject *object =
      new_instance(ARGON_TUPLE_TYPE, argc * sizeof(ArgonObject *));
  object->type = TYPE_TUPLE;
  object->value.as_tuple.size = argc;
  memcpy(object->value.as_tuple.data, argv, argc * sizeof(ArgonObject *));

  return object;
}

ArgonObject *ARGON_TUPLE___new__(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj =
      new_instance(argv[0], (argc - 1) * sizeof(ArgonObject *));
  new_obj->type = TYPE_TUPLE;
  new_obj->value.as_tuple.size = (argc - 1);
  return new_obj;
}

ArgonObject *ARGON_TUPLE___init__(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;

  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__init__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  memcpy(argv[0]->value.as_tuple.data, argv + 1,
         (argc - 1) * sizeof(ArgonObject *));
  return ARGON_NULL;
}

ArgonObject *ARGON_TUPLE___string__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;

  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

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
  string[string_length] = ')';
  string_length += 1;

  ArgonObject *result = new_string_object(string, string_length, 0, 0);

  free(string);

  return result;
}

ArgonObject *ARGON_TUPLE_get_length(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "get_length expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0]->value.as_tuple.size);
}

ArgonObject *ARGON_TUPLE_set_length(size_t argc, ArgonObject **argv, ArErr *err,
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

ArgonObject *ARGON_TUPLE___getitem__(size_t argc, ArgonObject **argv,
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
  if (index < 0)
    index += argv[0]->value.as_tuple.size;
  if (index >= (int64_t)argv[0]->value.as_tuple.size || index < 0) {
    return api->throw_argon_error(err, "Index Error", "index out of range");
  }
  return argv[0]->value.as_tuple.data[index];
}

ArgonObject *ARGON_TUPLE___setitem__(size_t argc, ArgonObject **argv,
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
  if (index < 0)
    index += argv[0]->value.as_tuple.size;
  if (index >= (int64_t)argv[0]->value.as_tuple.size || index < 0) {
    return api->throw_argon_error(err, "Index Error", "index out of range");
  }
  argv[0]->value.as_tuple.data[index] = argv[2];
  return argv[0]->value.as_tuple.data[index];
}

void init_tuple_type() {
  ARGON_TUPLE_TYPE = new_class();
  add_builtin_field(ARGON_TUPLE_TYPE, __name__,
                    new_string_object_null_terminated("tuple"));
  add_builtin_field(
      ARGON_TUPLE_TYPE, __new__,
      create_argon_native_function("__new__", ARGON_TUPLE___new__));
  add_builtin_field(
      ARGON_TUPLE_TYPE, __init__,
      create_argon_native_function("__init__", ARGON_TUPLE___init__));
  add_builtin_field(
      ARGON_TUPLE_TYPE, __string__,
      create_argon_native_function("__string__", ARGON_TUPLE___string__));
  add_builtin_field(
      ARGON_TUPLE_TYPE, get_length,
      create_argon_native_function("get_length", ARGON_TUPLE_get_length));
  add_builtin_field(
      ARGON_TUPLE_TYPE, set_length,
      create_argon_native_function("set_length", ARGON_TUPLE_set_length));
  add_builtin_field(
      ARGON_TUPLE_TYPE, __getitem__,
      create_argon_native_function("__getitem__", ARGON_TUPLE___getitem__));
  add_builtin_field(
      ARGON_TUPLE_TYPE, __setitem__,
      create_argon_native_function("__setitem__", ARGON_TUPLE___setitem__));

  ARGON_TUPLE_CREATE =
      create_argon_native_function("TUPLE_CREATE", TUPLE_CREATE);
}