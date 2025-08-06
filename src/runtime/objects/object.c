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
#include <string.h>

ArgonObject *BASE_CLASS = NULL;

ArgonObject *new_object() {
  ArgonObject *object = ar_alloc(sizeof(ArgonObject));
  object->dict = createHashmap_GC();
  add_field(object, "__class__", ARGON_TYPE_TYPE);
  return object;
}

void add_field(ArgonObject *target, char *name, ArgonObject *object) {
  hashmap_insert_GC(target->dict,
                    siphash64_bytes(name, strlen(name), siphash_key), name,
                    object, 0);
}

ArgonObject *get_field_for_class(ArgonObject *target, char *name) {
  char *field = "__base__";
  while (target) {
    uint64_t hash = siphash64_bytes(name, strlen(name), siphash_key);
    ArgonObject *object = hashmap_lookup_GC(target->dict, hash);
    if (object)
      return object;
    hash = siphash64_bytes(field, strlen(field), siphash_key);
    target = hashmap_lookup_GC(target->dict, hash);
  }
  return NULL;
}

ArgonObject *get_field(ArgonObject *target, char *name, bool recursive) {
  char *field = "__class__";
  while (target) {
    uint64_t hash = siphash64_bytes(name, strlen(name), siphash_key);
    ArgonObject *object = hashmap_lookup_GC(target->dict, hash);
    if (!recursive || object)
      return object;
    hash = siphash64_bytes(field, strlen(field), siphash_key);
    target = hashmap_lookup_GC(target->dict, hash);
    field = "__base__";
  }
  return NULL;
}