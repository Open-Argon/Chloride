/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "slice.h"
#include "../../../err.h"
#include "../exceptions/exceptions.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../object.h"
#include "../string/string.h"
#include "../tuple/tuple.h"
#include <inttypes.h>
#include <stdint.h>

ArgonObject *ARGON_SLICE_TYPE;

ArgonObject *ARGON_SLICE_TYPE___init__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state,
                                       ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 2 || argc > 4) {
    *err = create_err(RuntimeError,
                      "__init__ expects 2 to 4 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  self->type = TYPE_SLICE;
  if (argc == 2) {
    add_builtin_field(self, start, ARGON_NULL);
    add_builtin_field(self, stop, argv[1]);
    add_builtin_field(self, step, ARGON_NULL);
  } else if (argc > 2) {
    add_builtin_field(self, start, argv[1]);
    add_builtin_field(self, stop, argv[2]);
    if (argc > 3) {
      add_builtin_field(self, step, argv[3]);
    } else {
      add_builtin_field(self, step, ARGON_NULL);
    }
  }
  return ARGON_NULL;
}

// Returns -1 on error, 0 on success
int slice_indices(ArgonObject *self, int64_t length, SliceIndices *out,
                         ArErr *err, ArgonNativeAPI *api) {
  int64_t start_i64, stop_i64, step_i64;

  ArgonObject *start_obj = get_builtin_field(self, start);
  if (start_obj != ARGON_NULL) {
    start_i64 = api->argon_to_i64(start_obj, err);
    if (api->is_error(err)) return -1;
  } else {
    start_i64 = 0;
  }

  ArgonObject *stop_obj = get_builtin_field(self, stop);
  if (stop_obj != ARGON_NULL) {
    stop_i64 = api->argon_to_i64(stop_obj, err);
    if (api->is_error(err)) return -1;
  } else {
    stop_i64 = INT64_MAX;
  }

  ArgonObject *step_obj = get_builtin_field(self, step);
  if (step_obj != ARGON_NULL) {
    step_i64 = api->argon_to_i64(step_obj, err);
    if (api->is_error(err)) return -1;
  } else {
    step_i64 = 1;
  }

  if (step_i64 == 0) {
    api->throw_argon_error(err, RuntimeError, "slice step cannot be zero");
    return -1;
  }

  int64_t final_start_i64, final_stop_i64;

  if (step_i64 > 0) {
    if (start_obj == ARGON_NULL) {
      final_start_i64 = 0;
    } else {
      if (start_i64 < 0) start_i64 += length;
      final_start_i64 = start_i64 < 0 ? 0 : (start_i64 > length ? length : start_i64);
    }
    if (stop_obj == ARGON_NULL) {
      final_stop_i64 = length;
    } else {
      if (stop_i64 < 0) stop_i64 += length;
      final_stop_i64 = stop_i64 < 0 ? 0 : (stop_i64 > length ? length : stop_i64);
    }
  } else {
    if (start_obj == ARGON_NULL) {
      final_start_i64 = length - 1;
    } else {
      if (start_i64 < 0) start_i64 += length;
      final_start_i64 = start_i64 < -1 ? -1 : (start_i64 >= length ? length - 1 : start_i64);
    }
    if (stop_obj == ARGON_NULL) {
      final_stop_i64 = -1;
    } else {
      if (stop_i64 < 0) stop_i64 += length;
      final_stop_i64 = stop_i64 < -1 ? -1 : (stop_i64 >= length ? length - 1 : stop_i64);
    }
  }

  out->start = final_start_i64;
  out->stop  = final_stop_i64;
  out->step  = step_i64;
  return 0;
}

ArgonObject *ARGON_SLICE_TYPE_indices(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err))
    return ARGON_NULL;

  ArgonObject *self = argv[0];
  int64_t length = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;

  SliceIndices indices;
  if (slice_indices(self, length, &indices, err, api) < 0)
    return ARGON_NULL;

  ArgonObject *result_start = api->i64_to_argon(indices.start);
  ArgonObject *result_stop  = api->i64_to_argon(indices.stop);
  ArgonObject *result_step  = api->i64_to_argon(indices.step);

  return TUPLE_CREATE(3, (ArgonObject *[]){result_start, result_stop, result_step},
                      err, state, api);
}

void init_slice_type() {
  ARGON_SLICE_TYPE = new_class();
  add_builtin_field(ARGON_SLICE_TYPE, __name__,
                    new_string_object_null_terminated("slice"));
  add_builtin_field(
      ARGON_SLICE_TYPE, __init__,
      create_argon_native_function("__init__", ARGON_SLICE_TYPE___init__));
  add_builtin_field(
      ARGON_SLICE_TYPE, indices,
      create_argon_native_function("indices", ARGON_SLICE_TYPE_indices));
}