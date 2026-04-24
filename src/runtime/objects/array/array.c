/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "array.h"
#include "../../../../include/ArgonTypes.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../../internals/dynamic_array_armem/darray_armem.h"
#include "../exceptions/exceptions.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../slice/slice.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARRAY_TYPE;
ArgonObject *ARRAY_ITERATOR_TYPE;
ArgonObject *ARGON_ARRAY_CREATE;

ArgonObject *ARRAY_CREATE(size_t argc, ArgonObject **argv, ArErr *err,
                          RuntimeState *state, ArgonNativeAPI *api) {
  (void)err;
  (void)state;
  (void)api;
  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;

  object->value.as_array = darray_armem_create();

  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), argc);

  memcpy(object->value.as_array->data, argv, argc * sizeof(ArgonObject *));

  return object;
}

ARGON_METHOD(ARRAY_TYPE, __new__, {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError, "__new__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  ArgonObject *iter_method = get_builtin_field_for_class(
      get_builtin_field(argv[1], __class__), __iter__, argv[1]);
  if (!iter_method)
    return api->throw_argon_error(err, RuntimeError,
                                  "Object doesn't have __iter__ method");
  ArgonObject *iter_obj = argon_call(iter_method, 0, NULL, err, state);

  ArgonObject *next_method = get_builtin_field_for_class(
      get_builtin_field(iter_obj, __class__), __next__, iter_obj);
  if (!next_method)
    return api->throw_argon_error(
        err, RuntimeError, "Iterator object doesn't have __next__ method");

  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;
  object->value.as_array = darray_armem_create();
  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), 0);

  while (true) {
    ArgonObject *item = argon_call(next_method, 0, NULL, err, state);
    if (err->ptr == StopIteration_instance) {
      err->ptr = ARGON_NULL;
      break;
    } else if (api->is_error(err)) {
      return ARGON_NULL;
    }
    darray_armem_insert(object->value.as_array, object->value.as_array->size,
                        &item);
  }

  return object;
})

ARGON_METHOD(ARRAY_TYPE, __string__, {
  (void)api;

  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (!is_being_repr)
    is_being_repr = createHashmap();

  if (hashmap_lookup(is_being_repr, (uint64_t)argv[0]))
    return new_string_object_null_terminated("[...]");

  hashmap_insert(is_being_repr, (uint64_t)argv[0], NULL, (void *)true, 0);

  darray_armem *array = argv[0]->value.as_array;

  size_t capacity = 32;
  char *string = checked_malloc(capacity);
  size_t string_length = 1;
  string[0] = '[';

  for (size_t i = 0; i < array->size; i++) {
    ArgonObject *item = *(ArgonObject **)darray_armem_get(array, i);
    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(item, __class__), __repr__, item);

    if (i) {
      char *string_obj = ", ";
      size_t length = strlen(string_obj);
      bool resized = false;
      while (capacity < string_length + length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, err, state);
      bool resized = false;
      while (capacity < string_length + string_object->value.as_str->length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_object->value.as_str->data,
             string_object->value.as_str->length);
      string_length += string_object->value.as_str->length;
    } else {
      char *string_obj = "<object>";
      size_t length = strlen(string_obj);
      bool resized = false;
      while (capacity < string_length + length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }
  }
  if (capacity < string_length + 1) {
    capacity += 1;
    string = realloc(string, capacity);
  }
  string[string_length] = ']';
  string_length += 1;

  ArgonObject *result = new_string_object(string, string_length, 0);

  free(string);
  hashmap_remove(is_being_repr, (uint64_t)argv[0]);
  if (!is_being_repr->count)
    hashmap_free(is_being_repr, NULL);
  return result;
})

ARGON_METHOD(ARRAY_TYPE, append, {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err = create_err(RuntimeError, "append expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  darray_armem_insert(argv[0]->value.as_array, UINT64_MAX, &argv[1]);
  return ARGON_NULL;
})

ARGON_METHOD(ARRAY_TYPE, of_size, {
  (void)state;
  (void)api;
  if (argc != 1) {
    *err = create_err(RuntimeError, "of_size expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  int64_t size = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;
  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;

  object->value.as_array = darray_armem_create();

  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), size);

  for (int64_t i = 0; i < size; i++) {
    ((ArgonObject **)object->value.as_array->data)[i] = ARGON_NULL;
  }

  return object;
})

ARGON_METHOD(ARRAY_TYPE, __contains__, {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__contains__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *object_class = get_builtin_field(argv[1], __class__);
  ArgonObject *object__equal__ =
      get_builtin_field_for_class(object_class, __equal__, argv[1]);
  if (!object__equal__) {
    ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
    *err = create_err(RuntimeError,
                      "Object of type '%.*s' is missing __equal__ method",
                      (int)cls___name__->value.as_str->length,
                      cls___name__->value.as_str->data);
    return ARGON_NULL;
  }
  darray_armem *arr = argv[0]->value.as_array;
  for (size_t i = 0; i < arr->size; i++) {
    ArgonObject *result =
        argon_call(object__equal__, 1,
                   (ArgonObject *[]){*(ArgonObject **)darray_armem_get(arr, i)},
                   err, state);
    if (api->is_error(err))
      return ARGON_NULL;
    if (result == ARGON_TRUE)
      return result;
  }
  return ARGON_FALSE;
})

ARGON_METHOD(ARRAY_TYPE, get_length, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "get_length expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0]->value.as_array->size);
})

ARGON_METHOD(ARRAY_TYPE, set_length, {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "set_length expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  *err = create_err(RuntimeError, "attribute 'length' is immutable");
  return ARGON_NULL;
})

ARGON_METHOD(ARRAY_TYPE, join, {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 2) {
    *err = create_err(RuntimeError, "join expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  struct string join = api->argon_to_string(argv[1], err);

  size_t length = 0;

  for (size_t i = 0; i < argv[0]->value.as_array->size; i++) {
    ArgonObject *item =
        *(ArgonObject **)darray_armem_get(argv[0]->value.as_array, i);
    if (item->type != TYPE_STRING) {
      return api->throw_argon_error(err, ValueError,
                                    "index %" PRId64 " contains non string", i);
    }
    length += item->value.as_str->length;
    if (i != argv[0]->value.as_array->size - 1)
      length += join.length;
  }

  char *data = checked_malloc(length);
  char *p = data;

  for (size_t i = 0; i < argv[0]->value.as_array->size; i++) {
    ArgonObject *item =
        *(ArgonObject **)darray_armem_get(argv[0]->value.as_array, i);
    memcpy(p, item->value.as_str->data, item->value.as_str->length);
    p += item->value.as_str->length;
    if (i != argv[0]->value.as_array->size - 1) {
      memcpy(p, join.data, join.length);
      p += join.length;
    }
  }

  ArgonObject *result = api->string_to_argon((struct string){data, length});
  free(data);
  return result;
})

ARGON_METHOD(ARRAY_TYPE, insert, {
  (void)state;
  if (argc != 3) {
    *err = create_err(RuntimeError, "insert expects 3 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  size_t pos = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  darray_armem_insert(argv[0]->value.as_array, pos, &argv[2]);
  return ARGON_NULL;
})

ARGON_METHOD(ARRAY_TYPE, pop, {
  (void)state;
  if (argc > 2 || argc < 1) {
    *err = create_err(RuntimeError,
                      "pop expects 1 or 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  size_t pos = UINT64_MAX;
  if (argc == 2)
    pos = api->argon_to_i64(argv[1], err);
  darray_armem *arr = argv[0]->value.as_array;
  if (api->is_error(err))
    return ARGON_NULL;
  ArgonObject *out;
  if (!darray_armem_pop(arr, pos, &out))
    return api->throw_argon_error(err, IndexError, "pop from empty array");
  return out;
})

ARGON_METHOD(ARRAY_TYPE, __getitem__, {
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__getitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  darray_armem *arr = argv[0]->value.as_array;
  if (argv[1]->type == TYPE_SLICE) { // slice

    SliceIndices indices;
    if (slice_indices(argv[1], arr->size, &indices, err, api) != 0)
      return ARGON_NULL;

    ArgonObject *slice = new_instance(ARRAY_TYPE, sizeof(darray_armem));
    slice->type = TYPE_ARRAY;

    slice->value.as_array = darray_armem_create();

    int64_t size = 0;
    if (indices.step > 0 && indices.stop > indices.start)
      size = (indices.stop - indices.start + indices.step - 1) / indices.step;
    else if (indices.step < 0 && indices.stop < indices.start)
      size =
          (indices.start - indices.stop - indices.step - 1) / (-indices.step);

    darray_armem_init(slice->value.as_array, sizeof(ArgonObject *), size);

    for (int64_t i = 0; i < size; i++) {
      int64_t src_index =
          indices.start + i * indices.step; // source index follows step
      ((ArgonObject **)slice->value.as_array->data)[i] =
          *(ArgonObject **)darray_armem_get(arr, src_index);
    }

    return slice;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (index < 0)
    index += arr->size;
  if (index >= (int64_t)arr->size || index < 0) {
    return api->throw_argon_error(err, IndexError, "index out of range");
  }
  return *(ArgonObject **)darray_armem_get(arr, index);
})

ARGON_METHOD(ARRAY_TYPE, __setitem__, {
  (void)state;
  darray_armem *arr = argv[0]->value.as_array;
  if (argc != 3) {
    *err = create_err(RuntimeError,
                      "__setitem__ expects 3 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type == TYPE_SLICE) {
    SliceIndices indices;
    if (slice_indices(argv[1], arr->size, &indices, err, api) != 0)
      return ARGON_NULL;

    int64_t slice_len = 0;
    if (indices.step > 0 && indices.stop > indices.start)
      slice_len =
          (indices.stop - indices.start + indices.step - 1) / indices.step;
    else if (indices.step < 0 && indices.stop < indices.start)
      slice_len =
          (indices.start - indices.stop - indices.step - 1) / (-indices.step);

    // Collect new values from the iterable (argv[2])
    ArgonObject *iter_method = get_builtin_field_for_class(
        get_builtin_field(argv[2], __class__), __iter__, argv[2]);
    if (!iter_method)
      return api->throw_argon_error(err, RuntimeError,
                                    "Object doesn't have __iter__ method");
    ArgonObject *iter_obj = argon_call(iter_method, 0, NULL, err, state);
    if (api->is_error(err))
      return ARGON_NULL;

    ArgonObject *next_method = get_builtin_field_for_class(
        get_builtin_field(iter_obj, __class__), __next__, iter_obj);
    if (!next_method)
      return api->throw_argon_error(
          err, RuntimeError, "Iterator object doesn't have __next__ method");

    // Collect all new items into a temporary buffer
    darray_armem *new_items = darray_armem_create();
    darray_armem_init(new_items, sizeof(ArgonObject *), 0);

    while (true) {
      ArgonObject *item = argon_call(next_method, 0, NULL, err, state);
      if (err->ptr == StopIteration_instance) {
        err->ptr = ARGON_NULL;
        break;
      } else if (api->is_error(err)) {
        return ARGON_NULL;
      }
      darray_armem_insert(new_items, new_items->size, &item);
    }

    int64_t new_len = (int64_t)new_items->size;

    if (indices.step != 1) {
      // Extended slice: new iterable must match the slice length exactly
      if (new_len != slice_len) {
        return api->throw_argon_error(
            err, ValueError,
            "attempt to assign sequence of size %" PRId64
            " to extended slice of size %" PRId64,
            new_len, slice_len);
      }
      for (int64_t i = 0; i < slice_len; i++) {
        int64_t dst = indices.start + i * indices.step;
        ArgonObject *val = *(ArgonObject **)darray_armem_get(new_items, i);
        *(ArgonObject **)darray_armem_get(arr, dst) = val;
      }
    } else {
      // Simple slice (step == 1 or step == -1):
      // Normalize so we always work left-to-right with step == 1
      int64_t start = indices.start;
      if (indices.step == -1) {
        // reverse the new_items to match assignment order
        for (int64_t lo = 0, hi = new_len - 1; lo < hi; lo++, hi--) {
          ArgonObject **a = (ArgonObject **)darray_armem_get(new_items, lo);
          ArgonObject **b = (ArgonObject **)darray_armem_get(new_items, hi);
          ArgonObject *tmp = *a;
          *a = *b;
          *b = tmp;
        }
        // recalculate start for step==-1 slices: the affected region is
        // [stop+1 .. start] inclusive, left-to-right
        start = indices.stop + 1;
      }

      int64_t del_count = slice_len; // elements being removed
      int64_t old_size = (int64_t)arr->size;
      int64_t new_size = old_size - del_count + new_len;

      if (new_len < del_count) {
        // Shrink: shift tail left
        int64_t shift = del_count - new_len;
        for (int64_t i = start + new_len; i < new_size; i++) {
          *(ArgonObject **)darray_armem_get(arr, i) =
              *(ArgonObject **)darray_armem_get(arr, i + shift);
        }
        // Trim the array
        arr->size = (size_t)new_size;
      } else if (new_len > del_count) {
        // Grow: insert extra slots by shifting tail right
        int64_t shift = new_len - del_count;
        // Expand backing storage
        for (int64_t i = 0; i < shift; i++)
          darray_armem_insert(arr, arr->size, &ARGON_NULL);
        // Shift existing tail to the right
        for (int64_t i = new_size - 1; i >= start + new_len; i--) {
          *(ArgonObject **)darray_armem_get(arr, i) =
              *(ArgonObject **)darray_armem_get(arr, i - shift);
        }
      }

      // Write new values into [start .. start+new_len)
      for (int64_t i = 0; i < new_len; i++) {
        ArgonObject *val = *(ArgonObject **)darray_armem_get(new_items, i);
        *(ArgonObject **)darray_armem_get(arr, start + i) = val;
      }
    }

    return argv[2];
  }

  // --- integer index ---
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (index < 0)
    index += arr->size;
  if (index >= (int64_t)arr->size || index < 0)
    return api->throw_argon_error(err, IndexError, "index out of range");

  *(ArgonObject **)darray_armem_get(arr, index) = argv[2];
  return argv[2];
})

ARGON_METHOD(ARRAY_TYPE, __delitem__, {
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__delitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  darray_armem *arr = argv[0]->value.as_array;

  if (argv[1]->type == TYPE_SLICE) {
    SliceIndices indices;
    if (slice_indices(argv[1], arr->size, &indices, err, api) != 0)
      return ARGON_NULL;

    int64_t slice_len = 0;
    if (indices.step > 0 && indices.stop > indices.start)
      slice_len =
          (indices.stop - indices.start + indices.step - 1) / indices.step;
    else if (indices.step < 0 && indices.stop < indices.start)
      slice_len =
          (indices.start - indices.stop - indices.step - 1) / (-indices.step);

    if (slice_len == 0)
      return ARGON_NULL;

    if (indices.step == 1 || indices.step == -1) {
      // Simple slice: contiguous region to remove
      int64_t start = (indices.step == 1) ? indices.start : indices.stop + 1;
      int64_t old_size = (int64_t)arr->size;
      int64_t new_size = old_size - slice_len;

      // Shift tail left over the deleted region
      for (int64_t i = start; i < new_size; i++)
        *(ArgonObject **)darray_armem_get(arr, i) =
            *(ArgonObject **)darray_armem_get(arr, i + slice_len);

      arr->size = (size_t)new_size;
    } else {
      // Extended slice: delete indices individually.
      // Collect the indices to delete, then compact in one pass.
      // Mark slots as NULL first, then squeeze them out.
      for (int64_t i = 0; i < slice_len; i++) {
        int64_t idx = indices.start + i * indices.step;
        *(ArgonObject **)darray_armem_get(arr, idx) = NULL;
      }

      int64_t write = 0;
      for (int64_t read = 0; read < (int64_t)arr->size; read++) {
        ArgonObject *slot = *(ArgonObject **)darray_armem_get(arr, read);
        if (slot != NULL)
          *(ArgonObject **)darray_armem_get(arr, write++) = slot;
      }
      arr->size = (size_t)write;
    }

    return ARGON_NULL;
  }

  // --- integer index ---
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (index < 0)
    index += arr->size;
  if (index >= (int64_t)arr->size || index < 0)
    return api->throw_argon_error(err, IndexError, "index out of range");

  // Shift everything after the deleted index one slot left
  for (int64_t i = index; i < (int64_t)arr->size - 1; i++)
    *(ArgonObject **)darray_armem_get(arr, i) =
        *(ArgonObject **)darray_armem_get(arr, i + 1);

  arr->size--;
  return ARGON_NULL;
})

ARGON_METHOD(ARRAY_TYPE, __iter__, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *iterator =
      new_instance(ARRAY_ITERATOR_TYPE, sizeof(struct as_array_iterator));
  iterator->type = TYPE_ARRAY_ITERATOR;
  iterator->value.as_array_iterator =
      (struct as_array_iterator *)((char *)iterator + sizeof(ArgonObject));
  iterator->value.as_array_iterator->current = 0;
  iterator->value.as_array_iterator->array = self->value.as_array;
  return iterator;
})

ARGON_METHOD(ARRAY_ITERATOR_TYPE, __next__, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__next__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_array_iterator *array_iterator = self->value.as_array_iterator;

  if (array_iterator->current >= array_iterator->array->size) {
    err->ptr = StopIteration_instance;
    return ARGON_NULL;
  }

  ArgonObject *value = *(ArgonObject **)darray_armem_get(
      array_iterator->array, array_iterator->current++);

  return value;
})

void init_array_type() {
  ARRAY_TYPE = new_class();
  add_builtin_field(ARRAY_TYPE, __name__,
                    new_string_object_null_terminated("array"));
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __new__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __string__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, append)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, insert)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, pop)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, get_length)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, set_length)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, join)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __getitem__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __setitem__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __delitem__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __contains__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, __iter__)
  MOUNT_ARGON_METHOD(ARRAY_TYPE, of_size)

  ARRAY_ITERATOR_TYPE = new_class();
  add_builtin_field(ARRAY_ITERATOR_TYPE, __next__,
                    create_argon_native_function(
                        "__next__", ARRAY_ITERATOR_TYPE___next__));

  ARGON_ARRAY_CREATE =
      create_argon_native_function("ARRAY_CREATE", ARRAY_CREATE);
}