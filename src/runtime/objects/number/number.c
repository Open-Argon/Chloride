/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "number.h"
#include "../functions/functions.h"
#include "../string/string.h"
#include <gmp-x86_64.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

ArgonObject *ARGON_NUMBER_TYPE;

#include <gmp.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* change SIGNIFICANT_DIGITS to taste (15 mimics double-ish behaviour) */
#define SIGNIFICANT_DIGITS 15

ArgonObject *ARGON_NUMBER_TYPE___string__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
    return NULL;
  }

  mpq_t *num = argv[0]->value.as_number;

  /* If denominator == 1, print numerator as full integer */
  if (mpz_cmp_ui(mpq_denref(*num), 1) == 0) {
    char *num_str =
        mpz_get_str(NULL, 10, mpq_numref(*num)); /* malloc'd by GMP */
    ArgonObject *result = new_string_object_null_terminated(num_str);
    free(num_str);
    return result;
  }

  /* Not an integer: use mpf to format with SIGNIFICANT_DIGITS precision */
  mpf_t f;
  mpf_init(f);
  mpf_set_q(f, *num); /* set mpf from mpq */

  mp_exp_t exp; /* exponent returned by mpf_get_str */
  /* Request SIGNIFICANT_DIGITS significant digits. If you want "max accurate",
   * pass 0. */
  char *mant = mpf_get_str(NULL, &exp, 10, SIGNIFICANT_DIGITS, f);
  /* For zero, mpf_get_str returns an empty string and exp == 0 per GMP docs. */
  if (mant == NULL) {
    mpf_clear(f);
    return new_string_object_null_terminated("0");
  }

  /* handle zero specially */
  if (mant[0] == '\0' || (mant[0] == '0' && mant[1] == '\0')) {
    free(mant);
    mpf_clear(f);
    return new_string_object_null_terminated("0");
  }

  /* mant may include a leading '-' according to some docs; detect sign */
  int negative = 0;
  char *digits = mant;
  if (mant[0] == '-') {
    negative = 1;
    digits = mant + 1;
  }

  size_t L = strlen(digits); /* number of digit characters returned */
  /* mpf_get_str represents value as 0.digits * 10^exp (i.e. assumed decimal
   * point after the leading zero) */
  /* For scientific-format exponent (1.d..eE) we use scientific_exponent = exp -
   * 1 */
  long scientific_exp = (long)exp - 1L;

  /* Decide whether to use fixed or scientific, mimic C's %g rule:
     use scientific if exponent < -4 or exponent >= SIGNIFICANT_DIGITS */
  int use_scientific =
      (scientific_exp < -4) || (scientific_exp >= SIGNIFICANT_DIGITS);

  /* Build output into dynamic buffer */
  /* Worst-case: sign + 1 digit + '.' + (SIGNIFICANT_DIGITS-1) digits + 'e' +
   * sign + exponent digits + NUL */
  size_t buf_size = (size_t)(negative ? 1 : 0) + 1 + 1 +
                    (SIGNIFICANT_DIGITS - 1) + 1 + 1 + 32 + 1;
  /* For fixed form we may need more if exp > L (we append zeros). Allocate a
   * bit extra. */
  if (!use_scientific) {
    /* maximum integer digits = max(exp, L) but exp could be large; be
     * conservative */
    buf_size += (size_t)((exp > (mp_exp_t)L) ? (size_t)exp : L) + 16;
  }
  char *out = malloc(buf_size);
  if (!out) {
    free(mant);
    mpf_clear(f);
    *err = create_err(0, 0, 0, "", "Runtime Error", "out of memory");
    return NULL;
  }

  char *p = out;
  if (negative) {
    *p++ = '-';
  }

  if (use_scientific) {
    /* scientific:  d.dddddeE  where d = digits[0], fractional = digits[1..L-1]
     */
    *p++ = digits[0];
    if (L > 1) {
      *p++ = '.';
      memcpy(p, digits + 1, L - 1);
      p += L - 1;
    }
    /* append exponent */
    int written = snprintf(p, buf_size - (p - out), "e%+ld", scientific_exp);
    if (written < 0)
      written = 0;
    p += written;
  } else {
    /* fixed form: move decimal point right by 'exp' places in 0.digits * 10^exp
     */
    /* integer part length = exp (may be <=0 meaning 0) */
    long int_len = (long)exp;
    if (int_len <= 0) {
      /* 0.xxx... form */
      *p++ = '0';
      *p++ = '.';
      /* need (-int_len) leading zeros after decimal */
      for (long i = 0; i < -int_len; ++i)
        *p++ = '0';
      /* then digits */
      memcpy(p, digits, L);
      p += L;
    } else {
      /* integer part uses first int_len digits of digits (if available), else
       * digits plus zeros */
      if ((size_t)int_len <= L) {
        /* put first int_len digits as integer part */
        memcpy(p, digits, (size_t)int_len);
        p += int_len;
        /* fractional part exists if L > int_len */
        if (L > (size_t)int_len) {
          *p++ = '.';
          memcpy(p, digits + int_len, L - int_len);
          p += L - int_len;
        }
      } else {
        /* digits provide only part of integer, append zeros */
        memcpy(p, digits, L);
        p += L;
        for (long i = 0; i < int_len - (long)L; ++i)
          *p++ = '0';
        /* no fractional part */
      }
    }
  }

  *p = '\0';

  /* Clean up */
  free(mant);
  mpf_clear(f);

  ArgonObject *result = new_string_object_null_terminated(out);
  free(out);
  return result;
}

void create_ARGON_NUMBER_TYPE() {
  ARGON_NUMBER_TYPE = new_object();
  add_field(ARGON_NUMBER_TYPE, "__name__",
            new_string_object_null_terminated("number"));
  add_field(
      ARGON_NUMBER_TYPE, "__string__",
      create_argon_native_function("__string__", ARGON_NUMBER_TYPE___string__));
}

void mpz_init_gc_managed(mpz_t z, size_t limbs_count) {
  z->_mp_alloc = limbs_count;
  z->_mp_size = 0;
  z->_mp_d = ar_alloc(limbs_count * sizeof(mp_limb_t));
}

void mpq_init_gc_managed(mpq_t q, size_t num_limbs, size_t den_limbs) {
  mpz_init_gc_managed(mpq_numref(q), num_limbs);
  mpz_init_gc_managed(mpq_denref(q), den_limbs);
  mpq_set_ui(q, 0, 1); // initialize denominator to 1
}

void mpq_copy_to_gc(mpq_t dest, const mpq_t src) {
  size_t num_limbs = (size_t)abs(mpq_numref(src)->_mp_size);
  size_t den_limbs = (size_t)abs(mpq_denref(src)->_mp_size);

  dest->_mp_num._mp_size = mpq_numref(src)->_mp_size;
  memcpy(dest->_mp_num._mp_d, mpq_numref(src)->_mp_d,
         num_limbs * sizeof(mp_limb_t));

  dest->_mp_den._mp_size = mpq_denref(src)->_mp_size;
  memcpy(dest->_mp_den._mp_d, mpq_denref(src)->_mp_d,
         den_limbs * sizeof(mp_limb_t));
}

mpq_t *mpq_new_gc_from(const mpq_t src) {
  mpq_t *dest = ar_alloc(sizeof(mpq_t));

  size_t num_limbs = (size_t)mpq_numref(src)->_mp_alloc;
  size_t den_limbs = (size_t)mpq_denref(src)->_mp_alloc;

  mpq_init_gc_managed(*dest, num_limbs, den_limbs);
  mpq_copy_to_gc(*dest, src);

  return dest;
}

ArgonObject *new_number_object(mpq_t number) {
  ArgonObject *object = new_object();
  add_field(object, "__class__", ARGON_NUMBER_TYPE);
  object->type = TYPE_NUMBER;
  object->value.as_number = mpq_new_gc_from(number);
  return object;
}

void load_number(Translated *translated, RuntimeState *state) {
  uint8_t to_register = pop_byte(translated, state);
  size_t num_size = pop_bytecode(translated, state);
  size_t num_pos = pop_bytecode(translated, state);
  size_t den_size = pop_bytecode(translated, state);
  size_t den_pos = pop_bytecode(translated, state);
  mpq_t r;
  mpq_init(r);
  mpz_t num;
  mpz_init(num);
  mpz_import(num, num_size, 1, 1, 0, 0,
             arena_get(&translated->constants, num_pos));
  mpq_set_num(r, num);
  mpz_clear(num);
  mpz_t den;
  mpz_init(den);
  mpz_import(den, den_size, 1, 1, 0, 0,
             arena_get(&translated->constants, den_pos));
  mpq_set_den(r, den);
  mpz_clear(den);

  state->registers[to_register] = new_number_object(r);
  mpq_clear(r);
}
