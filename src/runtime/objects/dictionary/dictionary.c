/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "dictionary.h"
#include "../../call/call.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARGON_DICTIONARY_TYPE = NULL;

ArgonObject *create_ARGON_DICTIONARY_TYPE___init__(size_t argc,
                                                   ArgonObject **argv,
                                                   ArErr *err,
                                                   RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__init__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  object->type = TYPE_DICTIONARY;
  object->value.as_hashmap = createHashmap_GC();
  return object;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___string__(size_t argc,
                                                     ArgonObject **argv,
                                                     ArErr *err,
                                                     RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__init__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  size_t string_length = 0;
  char *string = NULL;
  struct node_GC **keys;
  size_t keys_length;
  hashmap_GC_to_array(object->value.as_hashmap, &keys, &keys_length);
  char *string_obj = "{";
  size_t length = strlen(string_obj);
  string = realloc(string, string_length + length);
  memcpy(string + string_length, string_obj, length);
  string_length += length;
  for (size_t i = 0; i < keys_length; i++) {
    struct node_GC *node = keys[i];
    ArgonObject *key = node->key;
    ArgonObject *value = node->val;

    if (!key) { fprintf(stderr, "NULL key at node %zu\n", i); continue; }
    if (!value) { fprintf(stderr, "NULL value at node %zu\n", i); continue; }

    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(key, __class__), __repr__, key);

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, err, state);
      string =
          realloc(string, string_length + string_object->value.as_str->length);
      memcpy(string + string_length, string_object->value.as_str->data,
             string_object->value.as_str->length);
      string_length += string_object->value.as_str->length;
    } else {
      char *string_obj = "<object>";
      size_t length = strlen(string_obj);
      string = realloc(string, string_length + length);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }

    char *string_obj = ": ";
    size_t length = strlen(string_obj);
    string = realloc(string, string_length + length);
    memcpy(string + string_length, string_obj, length);
    string_length += length;

    string_convert_method = get_builtin_field_for_class(
        get_builtin_field(value, __class__), __repr__, value);

    if (string_convert_method && value != object) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, err, state);
      string =
          realloc(string, string_length + string_object->value.as_str->length);
      memcpy(string + string_length, string_object->value.as_str->data,
             string_object->value.as_str->length);
      string_length += string_object->value.as_str->length;
    } else {
      char *string_obj = "{...}";
      size_t length = strlen(string_obj);
      string = realloc(string, string_length + length);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }

    if (i != keys_length - 1) {
      char *string_obj = ", ";
      size_t length = strlen(string_obj);
      string = realloc(string, string_length + length);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }
  }
  string_obj = "}";
  length = strlen(string_obj);
  string = realloc(string, string_length + length);
  memcpy(string + string_length, string_obj, length);
  string_length += length;
  ArgonObject* result = new_string_object(string, string_length, 0, 0);
  free(string);
  return result;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___get_attr__(size_t argc,
                                                       ArgonObject **argv,
                                                       ArErr *err,
                                                       RuntimeState *state) {
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__get_attr__ expects 2 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  ArgonObject *key = argv[1];
  int64_t hash = hash_object(key, err, state);
  if (err->exists) {
    return ARGON_NULL;
  }
  ArgonObject *result = hashmap_lookup_GC(object->value.as_hashmap, hash);
  if (!result) {
    *err = create_err(0, 0, 0, NULL, "Attribute Error",
                      "Dictionary has no attribute '%.*s'", key->value.as_str->length, key->value.as_str->data);
    return ARGON_NULL;
  }
  return result;
}


ArgonObject *create_ARGON_DICTIONARY_TYPE___setattr__(size_t argc,
                                                       ArgonObject **argv,
                                                       ArErr *err,
                                                       RuntimeState *state) {
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__setattr__ expects 2 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  ArgonObject *key = argv[1];
  ArgonObject *value = argv[2];
  int64_t hash = hash_object(key, err, state);
  if (err->exists) {
    return ARGON_NULL;
  }
  hashmap_insert_GC(object->value.as_hashmap, hash, key, value, 0);
  return value;
}


ArgonObject *create_ARGON_DICTIONARY_TYPE___setitem__(size_t argc,
                                                       ArgonObject **argv,
                                                       ArErr *err,
                                                       RuntimeState *state) {
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__setitem__ expects 2 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  ArgonObject *key = argv[1];
  ArgonObject *value = argv[2];
  int64_t hash = hash_object(key, err, state);
  if (err->exists) {
    return ARGON_NULL;
  }
  hashmap_insert_GC(object->value.as_hashmap, hash, key, value, 0);
  return value;
}

void create_ARGON_DICTIONARY_TYPE() {
  ARGON_DICTIONARY_TYPE = new_class();
  add_builtin_field(ARGON_DICTIONARY_TYPE, __name__,
                    new_string_object_null_terminated("dictionary"));
  add_builtin_field(ARGON_DICTIONARY_TYPE, __init__,
                    create_argon_native_function(
                        "__init__", create_ARGON_DICTIONARY_TYPE___init__));
  add_builtin_field(
      ARGON_DICTIONARY_TYPE, __get_attr__,
      create_argon_native_function("__get_attr__",
                                   create_ARGON_DICTIONARY_TYPE___get_attr__));
  add_builtin_field(
      ARGON_DICTIONARY_TYPE, __setattr__,
      create_argon_native_function("__setattr__",
                                   create_ARGON_DICTIONARY_TYPE___setattr__));
  add_builtin_field(
      ARGON_DICTIONARY_TYPE, __setitem__,
      create_argon_native_function("__setitem__",
                                   create_ARGON_DICTIONARY_TYPE___setitem__));
  add_builtin_field(ARGON_DICTIONARY_TYPE, __string__,
                    create_argon_native_function(
                        "__string__", create_ARGON_DICTIONARY_TYPE___string__));
}

ArgonObject *create_dictionary(struct hashmap_GC *hashmap) {
  ArgonObject *object = new_instance(ARGON_DICTIONARY_TYPE);
  object->type = TYPE_DICTIONARY;
  object->value.as_hashmap = hashmap;
  return object;
}