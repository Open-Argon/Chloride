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


ArgonObject *new_object() {
  ArgonObject *object = ar_alloc(sizeof(ArgonObject));
  object->type = TYPE_OBJECT;
  object->dict = createHashmap_GC();
  add_field(object, "__class__", ARGON_TYPE_TYPE);
  add_field(object, "__base__", BASE_CLASS);
  object->as_bool = true;
  return object;
}

void add_field(ArgonObject *target, char *name, ArgonObject *object) {
  hashmap_insert_GC(target->dict,
                    siphash64_bytes(name, strlen(name), siphash_key), name,
                    object, 0);
}

ArgonObject *bind_object_to_function(ArgonObject *object,
                                     ArgonObject *function) {
  ArgonObject *bound_method_wrapper = new_object();
  bound_method_wrapper->type = TYPE_METHOD;
  add_field(bound_method_wrapper, "__class__", ARGON_METHOD_TYPE);
  add_field(bound_method_wrapper, "__binding__", object);
  add_field(bound_method_wrapper, "__function__", function);
  ArgonObject *function_name = get_field(function, "__name__", false, false);
  if (function_name)
    add_field(bound_method_wrapper, "__name__", function_name);
  return bound_method_wrapper;
}

ArgonObject *get_field_for_class_l(ArgonObject *target, char *name,
                                   size_t length, ArgonObject *binding_object) {
  char *field = "__base__";
  size_t field_size = strlen(field);
  while (target) {
    ArgonObject *object = get_field_l(target, name, length, false, false);
    if (object) {
      if ((object->type == TYPE_FUNCTION ||
           object->type == TYPE_NATIVE_FUNCTION) &&
          binding_object) {
        object = bind_object_to_function(binding_object, object);
      }
      return object;
    }
    target = get_field_l(target, field, field_size, false, false);
  }
  return NULL;
}

ArgonObject *get_field_l(ArgonObject *target, char *name, size_t length,
                         bool recursive, bool disable_method_wrapper) {
  if(!target|| !target->dict) return NULL;
  char *field = "__class__";
  size_t field_size = strlen(field);
  ArgonObject *object = hashmap_lookup_GC(
      target->dict, siphash64_bytes(name, length, siphash_key));
  if (!recursive || object)
    return object;
  ArgonObject *binding = target;
  if (disable_method_wrapper)
    binding = NULL;
  return get_field_for_class_l(
      hashmap_lookup_GC(target->dict,
                        siphash64_bytes(field, field_size, siphash_key)),
      name, length, binding);
}

ArgonObject *get_field(ArgonObject *target, char *name, bool recursive,
                       bool disable_method_wrapper) {
  return get_field_l(target, name, strlen(name), recursive,
                     disable_method_wrapper);
}
ArgonObject *get_field_for_class(ArgonObject *target, char *name,
                                 ArgonObject *binding_object) {
  return get_field_for_class_l(target, name, strlen(name), binding_object);
}