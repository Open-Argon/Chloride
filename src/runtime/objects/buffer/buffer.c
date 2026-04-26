/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "buffer.h"

#include "../../../err.h"
#include "../../../memory.h"
#include "../exceptions/exceptions.h"
#include "../slice/slice.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

ArgonObject *ARGON_BUFFER_TYPE = NULL;

ArgonObject *create_ARGON_BUFFER_object(size_t size) {
  ArgonObject *object =
      new_instance(ARGON_BUFFER_TYPE, sizeof(struct buffer));
  object->type = TYPE_BUFFER;
  object->value.as_buffer =
      (struct buffer *)((char *)object + sizeof(ArgonObject));
  object->value.as_buffer->data = ar_alloc(size);  // separate allocation
  object->value.as_buffer->size = size;
  return object;
}

ARGON_METHOD(ARGON_BUFFER_TYPE, from_string, {
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
})

ARGON_METHOD(ARGON_BUFFER_TYPE, to_string, {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  struct buffer buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->string_to_argon((struct string){buffer.data, buffer.size});
})

ARGON_METHOD(ARGON_BUFFER_TYPE, of_size, {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  size_t n = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return create_ARGON_BUFFER_object(n);
})

ARGON_METHOD(ARGON_BUFFER_TYPE, extend, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;
  ArgonObject *self = argv[0];
  ArgonObject *other = argv[1];
  struct buffer other_buffer = api->argon_buffer_to_buffer(other, err);
  if (api->is_error(err))
    return api->ARGON_NULL;


  size_t self_size = self->value.as_buffer->size;
    
  resize_ARGON_BUFFER_object(self, err,
                            self_size + other_buffer.size);
  if (api->is_error(err))
    return api->ARGON_NULL;
  memcpy(self->value.as_buffer->data + self_size,
         other_buffer.data, other_buffer.size);
  return self;
})

ARGON_METHOD(ARGON_BUFFER_TYPE, get_length, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  struct buffer buffer = api->argon_buffer_to_buffer(argv[0], err);

  return api->i64_to_argon(buffer.size);
})

ARGON_METHOD(ARGON_BUFFER_TYPE, set_length, {
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "set_length expects 2 arguments, got %" PRIu64, argc);
    return api->ARGON_NULL;
  }

  *err = create_err(RuntimeError, "attribute 'length' is immutable");
  return api->ARGON_NULL;
})

ARGON_METHOD(ARGON_BUFFER_TYPE, __getitem__, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;
  struct buffer self = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  if (argv[1]->type == TYPE_SLICE) { // slice

    SliceIndices indices;
    if (slice_indices(argv[1], self.size, &indices, err, api) != 0)
      return api->ARGON_NULL;

    int64_t size = 0;
    if (indices.step > 0 && indices.stop > indices.start)
      size = (indices.stop - indices.start + indices.step - 1) / indices.step;
    else if (indices.step < 0 && indices.stop < indices.start)
      size =
          (indices.start - indices.stop - indices.step - 1) / (-indices.step);

    ArgonObject *slice_object = api->create_argon_buffer(size);
    struct buffer slice = api->argon_buffer_to_buffer(slice_object, err);

    for (int64_t i = 0; i < size; i++) {
      int64_t src_index =
          indices.start + i * indices.step; // source index follows step
      ((char *)slice.data)[i] = ((char *)self.data)[src_index];
    }
    return slice_object;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  if (index < 0)
    index += self.size;
  if (index >= (int64_t)self.size || index < 0) {
    return api->throw_argon_error(err, IndexError, "index out of range");
  }
  return api->i64_to_argon(((char *)self.data)[index]);
})

void resize_ARGON_BUFFER_object(ArgonObject *obj, ArErr *err, size_t new_size) {
  if (obj->type != TYPE_BUFFER) {
    *err = create_err(RuntimeError, "expected buffer object");
    return;
  }
  void *new_data = ar_realloc(obj->value.as_buffer->data, new_size);
  if (!new_data) {
    *err = create_err(RuntimeError,
                      "failed to reallocate the buffer to size %" PRIu64,
                      new_size);
    return;
  }
  obj->value.as_buffer->data = new_data;
  obj->value.as_buffer->size = new_size;
}

struct buffer ARGON_BUFFER_to_buffer_struct(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_BUFFER) {
    *err = create_err(RuntimeError, "expected buffer object");
    return (struct buffer){NULL, 0};
  }
  return *obj->value.as_buffer;
}

void create_ARGON_BUFFER_TYPE() {
  ARGON_BUFFER_TYPE = new_class();
  add_builtin_field(ARGON_BUFFER_TYPE, __name__,
                    new_string_object_null_terminated("buffer"));
  add_builtin_field(ARGON_BUFFER_TYPE, __new__, NULL);
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, of_size)
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, from_string)
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, to_string)
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, extend)
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, get_length)
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, set_length)
  MOUNT_ARGON_METHOD(ARGON_BUFFER_TYPE, __getitem__)
}