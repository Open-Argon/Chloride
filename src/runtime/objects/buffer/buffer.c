/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "buffer.h"
#include "../functions/functions.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

ArgonObject *ARGON_BUFFER_TYPE = NULL;

ArgonObject *create_ARGON_BUFFER_object(size_t size) {
  ArgonObject *object = new_instance(ARGON_BUFFER_TYPE);
  object->type = TYPE_BUFFER;
  object->value.as_buffer = ar_alloc(sizeof(struct buffer));
  object->value.as_buffer->data = ar_alloc_atomic(size);
  object->value.as_buffer->size = size;
  return object;
}

ArgonObject *ARGON_BUFFER_from_string(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  struct string str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  ArgonObject *buffer_object = create_ARGON_BUFFER_object(str.length);
  struct buffer buffer = api->argon_buffer_to_buffer(buffer_object, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  memcpy(buffer.data, str.data, buffer.size);
  return buffer_object;
}

ArgonObject *ARGON_BUFFER_to_string(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  struct buffer buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->string_to_argon((struct string){buffer.data, buffer.size});
}

ArgonObject *ARGON_BUFFER_of_size(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state, ArgonNativeAPI *api) {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  size_t n = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return create_ARGON_BUFFER_object(n);
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

struct buffer ARGON_BUFFER_to_buffer_struct(ArgonObject *obj, ArErr *err) {
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
  add_builtin_field(
      ARGON_BUFFER_TYPE, of_size,
      create_argon_native_function("of_size", ARGON_BUFFER_of_size));
  add_builtin_field(
      ARGON_BUFFER_TYPE, from_string,
      create_argon_native_function("from_string", ARGON_BUFFER_from_string));
  add_builtin_field(
      ARGON_BUFFER_TYPE, to_string,
      create_argon_native_function("to_string", ARGON_BUFFER_to_string));
}