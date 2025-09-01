/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "object.h"
#include "../../hash_data/hash_data.h"
#include "../../memory.h"
#include "type/type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

ArgonObject *BASE_CLASS = NULL;

const char *built_in_field_names[BUILT_IN_FIELDS_COUNT] = {
    "__base__",    "__class__",    "__name__",     "__add__",
    "__string__",  "__subtract__", "__multiply__", "__division__",
    "__new__",     "__init__",     "__boolean__",  "__get_attr__",
    "__binding__", "__function__", "address",      "__call__",
    "__number__",  "log"};

uint64_t built_in_field_hashes[BUILT_IN_FIELDS_COUNT];

ArgonObject *new_object() {
  ArgonObject *object = ar_alloc(sizeof(ArgonObject));
  object->type = TYPE_OBJECT;
  object->dict = createHashmap_GC();
  add_builtin_field(object, __class__, ARGON_TYPE_TYPE);
  add_builtin_field(object, __base__, BASE_CLASS);
  object->as_bool = true;
  return object;
}

void init_built_in_field_hashes() {
  for (int i = 0; i < BUILT_IN_FIELDS_COUNT; i++) {
    built_in_field_hashes[i] = siphash64_bytes(
        built_in_field_names[i], strlen(built_in_field_names[i]), siphash_key);
  }
}

void add_builtin_field(ArgonObject *target, built_in_fields field,
                       ArgonObject *object) {
  hashmap_insert_GC(target->dict, built_in_field_hashes[field],
                    (char *)built_in_field_names[field], object, 0);
}

void add_field(ArgonObject *target, char *name, uint64_t hash,
               ArgonObject *object) {
  hashmap_insert_GC(target->dict, hash, name, object, 0);
}

ArgonObject *bind_object_to_function(ArgonObject *object,
                                     ArgonObject *function) {
  ArgonObject *bound_method_wrapper = new_object();
  bound_method_wrapper->type = TYPE_METHOD;
  add_builtin_field(bound_method_wrapper, __class__, ARGON_METHOD_TYPE);
  add_builtin_field(bound_method_wrapper, __binding__, object);
  add_builtin_field(bound_method_wrapper, __function__, function);
  ArgonObject *function_name =
      get_builtin_field(function, __name__, false, false);
  if (function_name)
    add_builtin_field(bound_method_wrapper, __name__, function_name);
  return bound_method_wrapper;
}

ArgonObject *get_field_for_class_l(ArgonObject *target, char *name,
                                   uint64_t hash, size_t length,
                                   ArgonObject *binding_object) {
  while (target) {
    ArgonObject *object = get_field_l(target, name, hash, length, false, false);
    if (object) {
      if ((object->type == TYPE_FUNCTION ||
           object->type == TYPE_NATIVE_FUNCTION) &&
          binding_object) {
        object = bind_object_to_function(binding_object, object);
      }
      return object;
    }
    target = get_builtin_field(target, __base__, false, false);
  }
  return NULL;
}

ArgonObject *get_field_l(ArgonObject *target, char *name, uint64_t hash,
                         size_t length, bool recursive,
                         bool disable_method_wrapper) {
  ArgonObject *object = hashmap_lookup_GC(target->dict, hash);
  if (!recursive || object)
    return object;
  ArgonObject *binding = target;
  if (disable_method_wrapper)
    binding = NULL;
  return get_field_for_class_l(
      hashmap_lookup_GC(target->dict, built_in_field_hashes[__class__]), name,
      hash, length, binding);
}

ArgonObject *get_builtin_field_for_class(ArgonObject *target,
                                         built_in_fields field,
                                         ArgonObject *binding_object) {
  while (target) {
    ArgonObject *object = get_builtin_field(target, field, false, false);
    if (object) {
      if ((object->type == TYPE_FUNCTION ||
           object->type == TYPE_NATIVE_FUNCTION) &&
          binding_object) {
        object = bind_object_to_function(binding_object, object);
      }
      return object;
    }
    target = get_builtin_field(target, __base__, false, false);
  }
  return NULL;
}

ArgonObject *get_builtin_field(ArgonObject *target, built_in_fields field,
                               bool recursive, bool disable_method_wrapper) {
  ArgonObject *object =
      hashmap_lookup_GC(target->dict, built_in_field_hashes[field]);
  if (!recursive || object)
    return object;
  ArgonObject *binding = target;
  if (disable_method_wrapper)
    binding = NULL;
  return get_builtin_field_for_class(
      hashmap_lookup_GC(target->dict, built_in_field_hashes[__class__]), field,
      binding);
}