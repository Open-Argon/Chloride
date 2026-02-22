/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "object.h"
#include "../../err.h"
#include "../../hash_data/hash_data.h"
#include "../../memory.h"
#include "../call/call.h"
#include "type/type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int strcmp_len(const char *s1, size_t len, const char *s2) {
  size_t s2len = strlen(s2); // length of null-terminated string
  size_t n = (len < s2len) ? len : s2len;

  int cmp = memcmp(s1, s2, n);
  if (cmp != 0)
    return cmp;

  // If prefixes match, decide based on length
  if (len == s2len)
    return 0;
  return (len < s2len) ? -1 : 1;
}

ArgonObject *BASE_CLASS = NULL;

const char *built_in_field_names[BUILT_IN_FIELDS_COUNT] = {
    "__base__",
    "__class__",
    "__name__",
    "", // above is anything that gets stored in built in slots
    "__binding__",
    "__function__",
    "__add__",
    "__string__",
    "__subtract__",
    "__multiply__",
    "__exponent__",
    "__division__",
    "__floor_division__",
    "__modulo__",
    "__equal__",
    "__not_equal__",
    "__less_than__",
    "__less_than_equal__",
    "__greater_than__",
    "__greater_than_equal__",
    "__new__",
    "__init__",
    "__boolean__",
    "__getattr__",
    "address",
    "__call__",
    "__number__",
    "get_length",
    "set_length",
    "of_size",
    "from_string",
    "to_string",
    "__getattribute__",
    "__negation__",
    "__setattr__",
    "__getitem__",
    "__setitem__",
    "__hash__",
    "__repr__",
    "append",
    "insert",
    "pop",
    "__contains__",
    "__round__"};

uint64_t built_in_field_hashes[BUILT_IN_FIELDS_COUNT];

ArgonObject *new_object(size_t endSize) {
  ArgonObject *object = ar_alloc(sizeof(ArgonObject) + endSize);
  object->built_in_slot_length = 0;
  object->type = TYPE_OBJECT;
  object->dict = NULL;
  object->as_bool = true;
  return object;
}

void init_built_in_field_hashes() {
  for (int i = 0; i < BUILT_IN_FIELDS_COUNT; i++) {
    built_in_field_hashes[i] = siphash64_bytes(
        built_in_field_names[i], strlen(built_in_field_names[i]), siphash_key);
  }
}

int64_t hash_object(ArgonObject *object, ArErr *err, RuntimeState *state) {
  ArgonObject *hash_function = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __hash__, object);
  if (!hash_function) {
    return (int64_t)object;
  }
  ArgonObject *hash_result = argon_call(hash_function, 0, NULL, err, state);
  if (hash_result->type != TYPE_NUMBER ||
      !hash_result->value.as_number->is_int64) {
    *err =
        create_err(err->line, err->column, err->length, err->path, "Hash Error",
                   "hash result needs to be a 64 bit integer.");
    return 0;
  }
  return hash_result->value.as_number->n.i64;
}

ArgonObject *new_class() {
  ArgonObject *object = new_object(0);
  add_builtin_field(object, __class__, ARGON_TYPE_TYPE);
  add_builtin_field(object, __base__, BASE_CLASS);
  return object;
}

// ArgonObject *new_small_instance(ArgonObject *of, size_t endSize) {
//   ArgonObject *object = new_object(endSize);
//   add_builtin_field(object, __class__, of);
//   return object;
// }

ArgonObject *new_instance(ArgonObject *of, size_t endSize) {
  ArgonObject *object = new_object(endSize);
  add_builtin_field(object, __class__, of);
  return object;
}

inline void add_builtin_field(ArgonObject *target, built_in_fields field,
                              ArgonObject *object) {
  // pthread_rwlock_rdlock(&target->lock);
  for (size_t i = 0; i < target->built_in_slot_length; i++) {
    if (target->built_in_slot[i].field == field) {
      target->built_in_slot[i].value = object;
      // pthread_rwlock_unlock(&target->lock);
      return;
    }
  }
  if (field > BUILT_IN_ARRAY_COUNT) {
    if (!target->dict) {
      // pthread_rwlock_unlock(&target->lock);
      // pthread_rwlock_wrlock(&target->lock);
      target->dict = createHashmap_GC();
    }
    // pthread_rwlock_unlock(&target->lock);
    hashmap_insert_GC(target->dict, built_in_field_hashes[field],
                      (char *)built_in_field_names[field], object, 0);
    return;
  }
  // pthread_rwlock_unlock(&target->lock);
  // pthread_rwlock_wrlock(&target->lock);
  target->built_in_slot[target->built_in_slot_length++] =
      (struct built_in_slot){field, object};
  // pthread_rwlock_unlock(&target->lock);
  // hashmap_insert_GC(target->dict, built_in_field_hashes[field],
  //                   (char *)built_in_field_names[field], object, 0);
}

void add_field_l(ArgonObject *target, char *name, uint64_t hash, size_t length,
                 ArgonObject *object) {
  // pthread_rwlock_rdlock(&target->lock);
  for (size_t i = 0; i < BUILT_IN_ARRAY_COUNT; i++) {
    if (strcmp_len(name, length, built_in_field_names[i]) == 0) {
      // pthread_rwlock_unlock(&target->lock);
      add_builtin_field(target, i, object);
      return;
    }
  }
  if (!target->dict) {
    // pthread_rwlock_unlock(&target->lock);
    // pthread_rwlock_wrlock(&target->lock);
    target->dict = createHashmap_GC();
  }
  // pthread_rwlock_unlock(&target->lock);
  char *name_copy = ar_alloc_atomic(length);
  memcpy(name_copy, name, length);
  name_copy[length] = '\0';
  hashmap_insert_GC(target->dict, hash, name_copy, object, 0);
}

ArgonObject *bind_object_to_function(ArgonObject *object,
                                     ArgonObject *function) {
  ArgonObject *bound_method_wrapper = new_instance(ARGON_METHOD_TYPE, 0);
  bound_method_wrapper->type = TYPE_METHOD;
  add_builtin_field(bound_method_wrapper, __binding__, object);
  add_builtin_field(bound_method_wrapper, __function__, function);
  ArgonObject *function_name = get_builtin_field(function, __name__);
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
    target = get_builtin_field(target, __base__);
  }
  return NULL;
}

ArgonObject *get_field_l(ArgonObject *target, char *name, uint64_t hash,
                         size_t length, bool recursive,
                         bool disable_method_wrapper) {
  // pthread_rwlock_wrlock(&target->lock);
  for (size_t i = 0; i < target->built_in_slot_length; i++) {
    if (strcmp_len(name, length, built_in_field_names[i]) == 0) {
      ArgonObject *result = target->built_in_slot[i].value;
      // pthread_rwlock_unlock(&target->lock);
      return result;
    }
  }
  // pthread_rwlock_unlock(&target->lock);
  if (target->dict) {
    ArgonObject *object = hashmap_lookup_GC(target->dict, hash);
    if (!recursive || object)
      return object;
  }
  if (!recursive) {
    return NULL;
  }
  ArgonObject *binding = target;
  if (disable_method_wrapper)
    binding = NULL;
  return get_field_for_class_l(get_builtin_field(target, __class__), name, hash,
                               length, binding);
}

ArgonObject *get_builtin_field_for_class(ArgonObject *target,
                                         built_in_fields field,
                                         ArgonObject *binding_object) {
  while (target) {
    ArgonObject *object = get_builtin_field(target, field);
    if (object) {
      if ((object->type == TYPE_FUNCTION ||
           object->type == TYPE_NATIVE_FUNCTION) &&
          binding_object) {
        object = bind_object_to_function(binding_object, object);
      }
      return object;
    }
    target = get_builtin_field(target, __base__);
  }
  return NULL;
}
inline ArgonObject *get_builtin_field(ArgonObject *target,
                                      built_in_fields field) {
  return get_builtin_field_with_recursion_support(target, field, false, false);
}

ArgonObject *
get_builtin_field_with_recursion_support(ArgonObject *target,
                                         built_in_fields field, bool recursive,
                                         bool disable_method_wrapper) {
  if (!target)
    return NULL;
  // pthread_rwlock_wrlock(&target->lock);
  for (size_t i = 0; i < target->built_in_slot_length; i++) {
    if (target->built_in_slot[i].field == field) {
      ArgonObject *result = target->built_in_slot[i].value;
      // pthread_rwlock_unlock(&target->lock);
      return result;
    }
  }
  // pthread_rwlock_unlock(&target->lock);
  if (!target->dict)
    return NULL;
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