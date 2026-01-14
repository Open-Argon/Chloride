/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "buffer.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>

ArgonObject *ARGON_BUFFER_TYPE = NULL;

ArgonObject *create_ARGON_BUFFER_object(size_t size) {
  ArgonObject *object = new_instance(ARGON_BUFFER_TYPE);
  object->type = TYPE_BUFFER;
  object->value.as_buffer = ar_alloc(sizeof(struct buffer));
  object->value.as_buffer->data = ar_alloc_atomic(size);
  object->value.as_buffer->size = size;
  return object;
}

void resize_ARGON_BUFFER_object(ArgonObject *obj, ArErr *err, size_t new_size) {
  if (obj->type != TYPE_BUFFER) {
    *err = create_err(0, 0, 0, "", "Runtime Error", "expected buffer object");
    return;
  }
  void *new_data = ar_realloc(obj->value.as_buffer->data, new_size);
  if (!new_data) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "failed to reallocate the buffer to size %" PRIu64,
                      new_size);
    return;
  }
  obj->value.as_buffer->data = new_data;
  obj->value.as_buffer->size = new_size;
}

struct buffer ARGON_BUFFER_to_buffer_struct(ArgonObject *obj,
                                                   ArErr *err) {
  if (obj->type != TYPE_BUFFER) {
    *err = create_err(0, 0, 0, "", "Runtime Error", "expected buffer object");
    return (struct buffer){NULL, 0};
  }
  return *obj->value.as_buffer;
}

void create_ARGON_BUFFER_TYPE() {
  ARGON_BUFFER_TYPE = new_class();
  add_builtin_field(ARGON_BUFFER_TYPE, __name__,
                    new_string_object_null_terminated("buffer"));
}