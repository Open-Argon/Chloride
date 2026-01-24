/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "api.h"
#include "../objects/buffer/buffer.h"
#include "../objects/functions/functions.h"
#include "../objects/literals/literals.h"
#include "../objects/number/number.h"
#include "../objects/string/string.h"
#include "../../err.h"
#include "../runtime.h"
#include <gmp.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

ArgonObject *throw_argon_error(ArErr *err, const char *type, const char *fmt,
                               ...) {
  va_list args;
  va_start(args, fmt);
  *err = vcreate_err(0, 0, 0, "", type, fmt, args);
  va_end(args);
  return ARGON_NULL;
}

bool fix_to_arg_size(size_t fix, size_t argc, ArErr *err) {
  if (fix != argc) {
    throw_argon_error(err, "Runtime Error",
                      "expects %" PRIu64 " argument(s), got %" PRIu64, fix,
                      argc);
    return true;
  }
  return false;
}

int64_t argon_to_i64(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_NUMBER) {
    throw_argon_error(err, "Runtime Error", "expected number");
    return 0;
  } else if (!obj->value.as_number->is_int64) {
    throw_argon_error(err, "Runtime Error", "number has to be i64");
    return 0;
  }
  return obj->value.as_number->n.i64;
}

struct rational argon_to_num_and_den(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_NUMBER) {
    throw_argon_error(err, "Runtime Error", "expected number");
    return (struct rational){0, 0};
  }

  if (obj->value.as_number->is_int64) {
    return (struct rational){obj->value.as_number->n.i64, 1};
  }

  mpq_t *q = obj->value.as_number->n.mpq;

  mpz_srcptr num = mpq_numref(*q);
  mpz_srcptr den = mpq_denref(*q);

  if (!mpz_fits_slong_p(num)) {
    throw_argon_error(err, "Runtime Error", "numerator does not fit int64");
    return (struct rational){0, 0};
  }

  if (!mpz_fits_ulong_p(den)) {
    throw_argon_error(err, "Runtime Error", "denominator does not fit uint64");
    return (struct rational){0, 0};
  }

  int64_t n = (int64_t)mpz_get_si(num);
  uint64_t d = (uint64_t)mpz_get_ui(den);

  return (struct rational){n, d};
}

double argon_to_double(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_NUMBER) {
    throw_argon_error(err, "Runtime Error", "expected number");
    return 0.0;
  }

  if (obj->value.as_number->is_int64) {
    return (double)obj->value.as_number->n.i64;
  }

  mpq_t *q = obj->value.as_number->n.mpq;

  double d = mpq_get_d(*q);

  if (!isfinite(d)) {
    throw_argon_error(err, "Runtime Error",
                      "number cannot be represented as double");
    return 0.0;
  }

  return d;
}

bool is_error(ArErr *err) { return err->exists; }

ArgonObject *rational_to_argon(struct rational r) {
  return new_number_object_from_num_and_den(r.n, r.d);
}

ArgonObject *string_to_argon(struct string str) {
  return new_string_object(str.data, str.length, 0, 0);
}

struct string argon_to_string(ArgonObject *obj, ArErr *err) {
  if (obj->type != TYPE_STRING) {
    throw_argon_error(err, "Runtime Error", "expected string");
    return (struct string){NULL, 0};
  }

  return (struct string){obj->value.as_str->data, obj->value.as_str->length};
}

ArgonNativeAPI native_api = {
    .register_ArgonObject = add_to_hashmap,
    .create_argon_native_function = create_argon_native_function,
    .throw_argon_error = throw_argon_error,
    .is_error = is_error,
    .fix_to_arg_size = fix_to_arg_size,

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
};