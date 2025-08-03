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