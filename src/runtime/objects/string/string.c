/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../../../err.h"
#include "../../api/api.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../object.h"
#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARGON_STRING_TYPE = NULL;

char *c_quote_string(const char *input, size_t len) {
  // Worst case: every byte becomes "\uXXXX" (6 chars) + quotes + NUL
  size_t max_out = 2 + (len * 6) + 1;
  char *out = malloc(max_out);
  if (!out)
    return NULL;

  size_t j = 0;
  out[j++] = '"';

  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)input[i];

    switch (c) {
    case '\n':
      out[j++] = '\\';
      out[j++] = 'n';
      break;
    case '\t':
      out[j++] = '\\';
      out[j++] = 't';
      break;
    case '\r':
      out[j++] = '\\';
      out[j++] = 'r';
      break;
    case '\\':
      out[j++] = '\\';
      out[j++] = '\\';
      break;
    case '\"':
      out[j++] = '\\';
      out[j++] = '\"';
      break;
    default:
      if (isprint(c)) {
        out[j++] = c;
      } else {
        // write \uXXXX
        j += sprintf(&out[j], "\\u%04X", c);
      }
    }
  }

  out[j++] = '"';
  out[j] = '\0';
  return out;
}

ArgonObject *ARGON_STRING_TYPE___equal__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err("Runtime Error",
                      "__equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return (argv[0]->type == TYPE_STRING && argv[1]->type == TYPE_STRING) &&
                 (argv[0]->value.as_str->hash == argv[1]->value.as_str->hash) &&
                 (argv[0]->value.as_str->length ==
                  argv[1]->value.as_str->length) &&
                 (memcmp(argv[0]->value.as_str->data,
                         argv[1]->value.as_str->data,
                         argv[0]->value.as_str->length) == 0)
             ? ARGON_TRUE
             : ARGON_FALSE;
}

ArgonObject *ARGON_STRING_TYPE___not_equal__(size_t argc, ArgonObject **argv,
                                             ArErr *err, RuntimeState *state,
                                             ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err("Runtime Error",
                      "__not_equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return ARGON_STRING_TYPE___equal__(argc, argv, err, state, api) == ARGON_TRUE
             ? ARGON_FALSE
             : ARGON_TRUE;
}

ArgonObject *ARGON_STRING_TYPE___less_than__(size_t argc, ArgonObject **argv,
                                             ArErr *err, RuntimeState *state,
                                             ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err("Runtime Error",
                      "__less_than__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err("Runtime Error", "__less_than__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp < 0)
    return ARGON_TRUE;
  if (cmp > 0)
    return ARGON_FALSE;

  return len1 < len2 ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *ARGON_STRING_TYPE___less_than_equal__(size_t argc,
                                                   ArgonObject **argv,
                                                   ArErr *err,
                                                   RuntimeState *state,
                                                   ArgonNativeAPI *api) {
  (void)api;
  (void)state;

  if (argc != 2) {
    *err = create_err("Runtime Error",
                      "__less_than_equal__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err("Runtime Error", "__less_than_equal__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp < 0)
    return ARGON_TRUE;
  if (cmp > 0)
    return ARGON_FALSE;

  return len1 <= len2 ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *ARGON_STRING_TYPE___greater_than__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api) {
  (void)api;
  (void)state;

  if (argc != 2) {
    *err =
        create_err("Runtime Error",
                   "__greater_than__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err("Runtime Error", "__greater_than__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp > 0)
    return ARGON_TRUE;
  if (cmp < 0)
    return ARGON_FALSE;

  return len1 > len2 ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *ARGON_STRING_TYPE___greater_than_equal__(size_t argc,
                                                      ArgonObject **argv,
                                                      ArErr *err,
                                                      RuntimeState *state,
                                                      ArgonNativeAPI *api) {

  (void)api;
  (void)state;

  if (argc != 2) {
    *err = create_err(
        "Runtime Error",
        "__greater_than_equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err =
        create_err("Runtime Error", "__greater_than_equal__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp > 0)
    return ARGON_TRUE;
  if (cmp < 0)
    return ARGON_FALSE;

  return len1 >= len2 ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *ARGON_STRING_TYPE_get_length(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err("Runtime Error",
                      "get_length expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0]->value.as_str->length);
}

ArgonObject *ARGON_STRING_TYPE_set_length(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 2) {
    *err = create_err("Runtime Error",
                      "set_length expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  *err = create_err("Runtime Error", "attribute 'length' is immutable");
  return ARGON_NULL;
}

void init_string(ArgonObject *object, char *data, size_t length,
                 uint64_t hash) {
  object->type = TYPE_STRING;
  object->value.as_str =
      (struct string_struct *)((char *)object + sizeof(ArgonObject));
  object->value.as_str->data = data;
  object->value.as_str->hash = hash;
  object->value.as_str->length = length;
  object->as_bool = length;
}

ArgonObject *new_string_object_without_memcpy(char *data, size_t length,
                                              uint64_t hash) {
  ArgonObject *object =
      new_small_instance(ARGON_STRING_TYPE, sizeof(struct string_struct));
  init_string(object, data, length, hash);
  return object;
}

ArgonObject *new_string_object(char *data, size_t length, uint64_t hash) {
  char *data_copy = ar_alloc_atomic(length);
  memcpy(data_copy, data, length);
  return new_string_object_without_memcpy(data_copy, length, hash);
}

char *argon_object_to_null_terminated_string(ArgonObject *object, ArErr *err,
                                             RuntimeState *state) {
  ArgonObject *string_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __repr__, object);

  if (!string_convert_method)
    return "<object>";

  ArgonObject *string_object =
      argon_call(string_convert_method, 0, NULL, err, state);
  if (native_api.is_error(err))
    return NULL;

  if (string_object->type != TYPE_STRING)
    return "<object>";

  char *string = ar_alloc(string_object->value.as_str->length + 1);
  string[string_object->value.as_str->length] = '\0';
  memcpy(string, string_object->value.as_str->data,
         string_object->value.as_str->length);
  return string;
}

char *argon_string_to_c_string_malloc(ArgonObject *object) {
  if (object->type != TYPE_STRING)
    return NULL;

  char *string = malloc(object->value.as_str->length + 1);
  string[object->value.as_str->length] = '\0';
  memcpy(string, object->value.as_str->data, object->value.as_str->length);
  return string;
}

ArgonObject *new_string_object_null_terminated(char *data) {
  return new_string_object(data, strlen(data), 0);
}

ArgonObject *ARGON_RENDER_TEMPLATE;

ArgonObject *RENDER_TEMPLATE(size_t argc, ArgonObject **argv, ArErr *err,
                             RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 1) {
    *err = create_err("Runtime Error",
                      "RENDER_TEMPLATE expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  size_t capacity = 128;
  char *string = checked_malloc(capacity);
  size_t string_length = 0;

  struct tuple_struct *tuple = &argv[0]->value.as_tuple;

  for (size_t i = 0; i < tuple->size; i++) {
    ArgonObject *item = tuple->data[i]->value.as_tuple.data[1];

    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(item, __class__), __string__, item);

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
  ArgonObject *result = new_string_object(string, string_length, 0);

  free(string);
  return result;
}