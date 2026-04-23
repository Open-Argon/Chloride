/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "dictionary.h"
#include "../../../err.h"
#include "../../call/call.h"
#include "../exceptions/exceptions.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../string/string.h"
#include "../tuple/tuple.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARGON_DICTIONARY_TYPE;
ArgonObject *ARGON_DICTIONARY_ITERATOR_TYPE;

ArgonObject *create_ARGON_DICTIONARY_TYPE___new__(size_t argc,
                                                  ArgonObject **argv,
                                                  ArErr *err,
                                                  RuntimeState *state,
                                                  ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError, "__new__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *get_dictionary = get_builtin_field_for_class(
      get_builtin_field(argv[1], __class__), __dictionary__, argv[1]);
  if (!get_dictionary)
    return api->throw_argon_error(err, RuntimeError,
                                  "Object doesn't have __dictionary__ method");
  ArgonObject *object = argon_call(get_dictionary, 0, NULL, err, state);
  if (object->type != TYPE_DICTIONARY)
    return api->throw_argon_error(
        err, RuntimeError,
        "Objects __dictionary__ method didn't return a dictionary");
  return object;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___string__(size_t argc,
                                                     ArgonObject **argv,
                                                     ArErr *err,
                                                     RuntimeState *state,
                                                     ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];

  if (!is_being_repr)
    is_being_repr = createHashmap();

  if (hashmap_lookup(is_being_repr, (uint64_t)object))
    return new_string_object_null_terminated("{...}");

  hashmap_insert(is_being_repr, (uint64_t)object, NULL, (void *)true, 0);
  size_t string_length = 0;
  char *string = NULL;
  size_t nodes_length;
  struct node_GC **nodes =
      hashmap_GC_to_array(object->value.as_hashmap, &nodes_length);
  char *string_obj = "{";
  size_t length = strlen(string_obj);
  string = realloc(string, string_length + length);
  memcpy(string + string_length, string_obj, length);
  string_length += length;

  for (size_t i = 0; i < nodes_length; i++) {
    struct node_GC *node = nodes[i];
    ArgonObject *key = node->key;
    ArgonObject *value = node->val;

    if (!key) {
      fprintf(stderr, "NULL key at node %zu\n", i);
      continue;
    }
    if (!value) {
      fprintf(stderr, "NULL value at node %zu\n", i);
      continue;
    }

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

    if (string_convert_method) {
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

    if (i != nodes_length - 1) {
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
  ArgonObject *result = new_string_object(string, string_length, 0);
  free(string);
  hashmap_remove(is_being_repr, (uint64_t)object);
  if (!is_being_repr->count)
    hashmap_free(is_being_repr, NULL);
  return result;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___dictionary__(size_t argc,
                                                         ArgonObject **argv,
                                                         ArErr *err,
                                                         RuntimeState *state,
                                                         ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "__dictionary__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return argv[0];
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___contains__(size_t argc,
                                                       ArgonObject **argv,
                                                       ArErr *err,
                                                       RuntimeState *state,
                                                       ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__contains__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  int64_t hash = hash_object(argv[1], err, state);
  if (is_error(err)) {
    return ARGON_NULL;
  }
  ArgonObject *result = hashmap_lookup_GC(argv[0]->value.as_hashmap, hash);
  return result == NULL ? ARGON_FALSE : ARGON_TRUE;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___getitem__(size_t argc,
                                                      ArgonObject **argv,
                                                      ArErr *err,
                                                      RuntimeState *state,
                                                      ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__getitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  ArgonObject *key = argv[1];
  int64_t hash = hash_object(key, err, state);
  if (is_error(err)) {
    return ARGON_NULL;
  }
  ArgonObject *result = hashmap_lookup_GC(object->value.as_hashmap, hash);
  if (!result) {
    char *object_str =
        argon_object_to_null_terminated_string(key, err, state);
    if (api->is_error(err))
      return ARGON_NULL;

    *err = create_err(AttributeError, "Dictionary has no item %s", object_str);
    return ARGON_NULL;
  }
  return result;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___setitem__(size_t argc,
                                                      ArgonObject **argv,
                                                      ArErr *err,
                                                      RuntimeState *state,
                                                      ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 3) {
    *err = create_err(RuntimeError,
                      "__setitem__ expects 3 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  ArgonObject *key = argv[1];
  ArgonObject *value = argv[2];
  int64_t hash = hash_object(key, err, state);
  if (is_error(err)) {
    return ARGON_NULL;
  }
  hashmap_insert_GC(object->value.as_hashmap, hash, key, value, 0);
  return value;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___delitem__(size_t argc,
                                                      ArgonObject **argv,
                                                      ArErr *err,
                                                      RuntimeState *state,
                                                      ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__delitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object = argv[0];
  ArgonObject *key = argv[1];
  int64_t hash = hash_object(key, err, state);
  if (is_error(err)) {
    return ARGON_NULL;
  }
  if (!hashmap_remove_GC(object->value.as_hashmap, hash)) {
    char *object_str =
        argon_object_to_null_terminated_string(key, err, state);
    if (api->is_error(err))
      return ARGON_NULL;

    *err = create_err(AttributeError, "Dictionary has no item %s", object_str);
    return ARGON_NULL;
  }
  return ARGON_NULL;
}

ArgonObject *create_ARGON_DICTIONARY_TYPE___iter__(size_t argc,
                                                   ArgonObject **argv,
                                                   ArErr *err,
                                                   RuntimeState *state,
                                                   ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];

  ArgonObject *iterator = new_instance(ARGON_DICTIONARY_ITERATOR_TYPE,
                                       sizeof(struct as_dictionary_iterator));
  iterator->type = TYPE_DICTIONARY_ITERATOR;
  iterator->value.as_dictionary_iterator =
      (struct as_dictionary_iterator *)((char *)iterator + sizeof(ArgonObject));

  iterator->value.as_dictionary_iterator->current = 0;

  iterator->value.as_dictionary_iterator->array = hashmap_GC_to_array(
      self->value.as_hashmap, &iterator->value.as_dictionary_iterator->size);

  return iterator;
}

ArgonObject *
create_ARGON_DICTIONARY_ITERATOR_TYPE___next__(size_t argc, ArgonObject **argv,
                                               ArErr *err, RuntimeState *state,
                                               ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];

  if (self->value.as_dictionary_iterator->size <=
      self->value.as_dictionary_iterator->current) {
      err->ptr = StopIteration_instance;
      return ARGON_NULL;
  }

  struct node_GC *node =
      self->value.as_dictionary_iterator
          ->array[self->value.as_dictionary_iterator->current++];

  return TUPLE_CREATE(2, (ArgonObject *[]){node->key, node->val}, err, state,
                      api);
}

void create_ARGON_DICTIONARY_TYPE() {
  ARGON_DICTIONARY_TYPE = new_class();
  add_builtin_field(ARGON_DICTIONARY_TYPE, __name__,
                    new_string_object_null_terminated("dictionary"));
  add_builtin_field(ARGON_DICTIONARY_TYPE, __new__,
                    create_argon_native_function(
                        "__new__", create_ARGON_DICTIONARY_TYPE___new__));

  ArgonObject *setter = create_argon_native_function(
      "__setitem__", create_ARGON_DICTIONARY_TYPE___setitem__);
  ArgonObject *getter = create_argon_native_function(
      "__getitem__", create_ARGON_DICTIONARY_TYPE___getitem__);
  ArgonObject *deleter = create_argon_native_function(
      "__delitem__", create_ARGON_DICTIONARY_TYPE___delitem__);

  add_builtin_field(ARGON_DICTIONARY_TYPE, __setitem__, setter);
  add_builtin_field(ARGON_DICTIONARY_TYPE, __getitem__, getter);
  add_builtin_field(ARGON_DICTIONARY_TYPE, __delitem__, deleter);
  add_builtin_field(ARGON_DICTIONARY_TYPE, __setattr__, setter);
  add_builtin_field(ARGON_DICTIONARY_TYPE, __getattr__, getter);
  add_builtin_field(ARGON_DICTIONARY_TYPE, __delattr__, deleter);
  add_builtin_field(ARGON_DICTIONARY_TYPE, __string__,
                    create_argon_native_function(
                        "__string__", create_ARGON_DICTIONARY_TYPE___string__));
  add_builtin_field(
      ARGON_DICTIONARY_TYPE, __contains__,
      create_argon_native_function("__contains__",
                                   create_ARGON_DICTIONARY_TYPE___contains__));
  add_builtin_field(ARGON_DICTIONARY_TYPE, __iter__,
                    create_argon_native_function(
                        "__iter__", create_ARGON_DICTIONARY_TYPE___iter__));
  add_builtin_field(
      ARGON_DICTIONARY_TYPE, __dictionary__,
      create_argon_native_function(
          "__dictionary__", create_ARGON_DICTIONARY_TYPE___dictionary__));

  ARGON_DICTIONARY_ITERATOR_TYPE = new_class();
  add_builtin_field(ARGON_DICTIONARY_ITERATOR_TYPE, __name__,
                    new_string_object_null_terminated("dictionary_iterator"));
  add_builtin_field(
      ARGON_DICTIONARY_ITERATOR_TYPE, __next__,
      create_argon_native_function(
          "__next__", create_ARGON_DICTIONARY_ITERATOR_TYPE___next__));
}

ArgonObject *create_dictionary(struct hashmap_GC *hashmap) {
  ArgonObject *object = new_instance(ARGON_DICTIONARY_TYPE, 0);
  object->type = TYPE_DICTIONARY;
  object->value.as_hashmap = hashmap;
  return object;
}