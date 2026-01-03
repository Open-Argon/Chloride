/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../../call/call.h"
#include "../number/number.h"
#include "../object.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
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

void init_string(ArgonObject *object, char *data, size_t length,
                 uint64_t prehash, uint64_t hash) {
  add_builtin_field(object, field_length, new_number_object_from_int64(length));
  object->type = TYPE_STRING;
  object->value.as_str = ar_alloc(sizeof(struct string_struct));
  object->value.as_str->data = data;
  object->value.as_str->prehash = prehash;
  object->value.as_str->hash_computed = hash;
  object->value.as_str->hash = hash;
  object->value.as_str->length = length;
  object->as_bool = length;
}

ArgonObject *new_string_object_without_memcpy(char *data, size_t length,
                                              uint64_t prehash, uint64_t hash) {
  ArgonObject *object = new_instance(ARGON_STRING_TYPE);
  init_string(object, data, length, prehash, hash);
  return object;
}

ArgonObject *new_string_object(char *data, size_t length, uint64_t prehash,
                               uint64_t hash) {
  char *data_copy = ar_alloc_atomic(length);
  memcpy(data_copy, data, length);
  return new_string_object_without_memcpy(data_copy, length, prehash, hash);
}

char *argon_object_to_null_terminated_string(ArgonObject *object, ArErr *err,
                                             RuntimeState *state) {
  ArgonObject *string_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __repr__, object);

  if (!string_convert_method)
    return "<object>";

  ArgonObject *string_object =
      argon_call(string_convert_method, 0, NULL, err, state);
  if (err->exists)
    return NULL;


  if (string_object->type != TYPE_STRING) return "<object>";

  char *string = ar_alloc(string_object->value.as_str->length+1);
  string[string_object->value.as_str->length] = '\0';
  memcpy(string, string_object->value.as_str->data, string_object->value.as_str->length);
  return string;
}

ArgonObject *new_string_object_null_terminated(char *data) {
  return new_string_object(data, strlen(data), 0, 0);
}