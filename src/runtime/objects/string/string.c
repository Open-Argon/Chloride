/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../number/number.h"
#include "../object.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

ArgonObject *ARGON_STRING_TYPE = NULL;

ArgonObject *new_string_object_without_memcpy(char *data, size_t length, uint64_t prehash,
                               uint64_t hash) {
  ArgonObject *object = new_object();
  add_builtin_field(object, __class__, ARGON_STRING_TYPE);
  add_builtin_field(object, field_length,
                    new_number_object_from_int64(length));
  object->type = TYPE_STRING;
  object->value.as_str.data = data;
  object->value.as_str.prehash = prehash;
  object->value.as_str.hash_computed = hash;
  object->value.as_str.hash = hash;
  object->value.as_str.length = length;
  object->as_bool = length;
  return object;
}

ArgonObject *new_string_object(char *data, size_t length, uint64_t prehash,
                               uint64_t hash) {
  char*data_copy = ar_alloc_atomic(length);
  memcpy(data_copy, data, length);
  return new_string_object_without_memcpy(data_copy,length, prehash, hash);
}

ArgonObject *new_string_object_null_terminated(char *data) {
  return new_string_object(data, strlen(data), 0, 0);
}