/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "api.h"
#include "../../RWLock.h"
#include "../../err.h"
#include "../../memory.h"
#include "../call/call.h"
#include "../objects/buffer/buffer.h"
#include "../objects/exceptions/exceptions.h"
#include "../objects/functions/functions.h"
#include "../objects/literals/literals.h"
#include "../objects/number/number.h"
#include "../objects/object.h"
#include "../objects/string/string.h"
#include <gmp.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

ArgonObject *throw_argon_error(ArErr *err, ArgonObject *type, const char *fmt,
                               ...) {
  va_list args;
  va_start(args, fmt);
  *err = vcreate_err(type, fmt, args);
  va_end(args);
  if (!is_instance(err->ptr, BaseException)) {
    *err = create_err(TypeError, "exceptions must derive from BaseException");
  }
  return ARGON_NULL;
}

ArgonObject *throw_argon_signal(ArErr *err, ArgonObject *signal) {
  if (!is_instance(signal, SignalException)) {
    *err = create_err(TypeError, "signals must derive from SignalException");
    return ARGON_NULL;
  }
  err->ptr = signal;
  return ARGON_NULL;
}

bool fix_to_arg_size(size_t fix, size_t argc, ArErr *err) {
  if (fix != argc) {
    throw_argon_error(err, RuntimeError,
                      "expects %" PRIu64 " argument(s), got %" PRIu64, fix,
                      argc);
    return true;
  }
  return false;
}

int64_t argon_to_i64(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_NUMBER) {
    throw_argon_error(err, RuntimeError, "expected number");
    return 0;
  } else if (!obj->value.as_number->is_int64) {
    throw_argon_error(err, RuntimeError, "number has to be i64");
    return 0;
  }
  return obj->value.as_number->n.i64;
}

struct rational argon_to_num_and_den(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_NUMBER) {
    throw_argon_error(err, RuntimeError, "expected number");
    return (struct rational){0, 0};
  }

  if (obj->value.as_number->is_int64) {
    return (struct rational){obj->value.as_number->n.i64, 1};
  }

  mpq_t *q = obj->value.as_number->n.mpq;

  mpz_srcptr num = mpq_numref(*q);
  mpz_srcptr den = mpq_denref(*q);

  if (!mpz_fits_slong_p(num)) {
    throw_argon_error(err, RuntimeError, "numerator does not fit int64");
    return (struct rational){0, 0};
  }

  if (!mpz_fits_ulong_p(den)) {
    throw_argon_error(err, RuntimeError, "denominator does not fit uint64");
    return (struct rational){0, 0};
  }

  int64_t n = (int64_t)mpz_get_si(num);
  uint64_t d = (uint64_t)mpz_get_ui(den);

  return (struct rational){n, d};
}

double argon_to_double(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_NUMBER) {
    throw_argon_error(err, RuntimeError, "expected number");
    return 0.0;
  }

  if (obj->value.as_number->is_int64) {
    return (double)obj->value.as_number->n.i64;
  }

  mpq_t *q = obj->value.as_number->n.mpq;

  double d = mpq_get_d(*q);

  if (!isfinite(d)) {
    throw_argon_error(err, RuntimeError,
                      "number cannot be represented as double");
    return 0.0;
  }

  return d;
}

ArgonObject *rational_to_argon(struct rational r) {
  return new_number_object_from_num_and_den(r.n, r.d);
}

ArgonObject *string_to_argon(struct string str) {
  return new_string_object(str.data, str.length, 0);
}

struct string argon_to_string(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_STRING) {
    throw_argon_error(err, RuntimeError, "expected string");
    return (struct string){NULL, 0};
  }

  return (struct string){obj->value.as_str->data, obj->value.as_str->length};
}

struct array argon_to_array(ArgonObject *obj, ArErr *err) {
  if (obj->type == TYPE_ARRAY) {
    return (struct array){obj->value.as_array->data, obj->value.as_array->size};
  } else if (obj->type == TYPE_TUPLE) {
    return (struct array){obj->value.as_tuple.data, obj->value.as_tuple.size};
  }
  throw_argon_error(err, RuntimeError, "expected array or tuple");
  return (struct array){NULL, 0};
}

int register_thread() {
  struct GC_stack_base sb;
  atomic_fetch_add(&thread_count, 1);
  GC_get_stack_base(&sb);
  return GC_register_my_thread(&sb);
}

int unregister_thread() {
  atomic_fetch_sub(&thread_count, 1);
  unregister_thread_pool();
  return GC_unregister_my_thread();
}

ArgonObject *create_err_object() {
  ArgonObject *obj = new_object(0);
  obj->type = TYPE_ERROR;
  obj->value.err = no_err;
  return obj;
}

ArErr *err_object_to_err(ArgonObject *object, ArErr *err) {
  if (object->type != TYPE_ERROR)
    *err = create_err(RuntimeError, "Expected error object");
  return &object->value.err;
}

void set_err(ArgonObject *object, ArErr *err) {
  if (object->type != TYPE_ERROR) {
    *err = create_err(RuntimeError, "Expected error object");
    return;
  }
  *err = object->value.err;
  return;
}

RuntimeState *new_state(ArgonObject **registers) {
  RuntimeState *new_state = ar_alloc(sizeof(RuntimeState));
  new_state->source_location = (SourceLocation){0, 0, 0};
  new_state->load_number_cache = createHashmap_GC();
  new_state->head = 0;
  new_state->path = "";
  // Ensure proper alignment
  new_state->registers = registers;
  return new_state;
}

int argon_to_argonType(ArgonObject *obj) { return obj->type; }

bool argon_is_i64(ArgonObject *obj) {
  return obj->type == TYPE_NUMBER && obj->value.as_number->is_int64;
}

ArgonNativeAPI native_api = {
    .register_ArgonObject = add_to_hashmap,
    .create_argon_native_function = create_argon_native_function,
    .throw_argon_error = throw_argon_error,
    .is_error = is_error,
    .fix_to_arg_size = fix_to_arg_size,
    .call = argon_call,

    .i64_to_argon = new_number_object_from_int64,
    .double_to_argon = new_number_object_from_double,
    .rational_to_argon = rational_to_argon,
    .argon_to_i64 = argon_to_i64,
    .argon_to_double = argon_to_double,
    .argon_to_rational = argon_to_num_and_den,

    .string_to_argon = string_to_argon,
    .argon_to_string = argon_to_string,

    .create_argon_buffer = create_ARGON_BUFFER_object,
    .resize_argon_buffer = resize_ARGON_BUFFER_object,
    .argon_buffer_to_buffer = ARGON_BUFFER_to_buffer_struct,
    .register_thread = register_thread,
    .unregister_thread = unregister_thread,
    .create_err_object = create_err_object,
    .err_object_to_err = err_object_to_err,
    .new_state = new_state,
    .set_err = set_err,
    .malloc = GC_malloc_uncollectable,
    .free = GC_free,
    .argon_to_array = argon_to_array,
    .argon_get_ArgonType = argon_to_argonType,
    .argon_is_i64 = argon_is_i64,

    .throw_argon_signal = throw_argon_signal};