/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "number.h"
#include "../../err.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include <ctype.h>
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Detect the base of a number literal from its prefix.
 *
 * "0x"/"0X" → base 16  (exponent char: p/P, exponent base: 2)
 * "0o"/"0O" → base 8   (exponent char: e/E, exponent base: 8)
 * "0b"/"0B" → base 2   (exponent char: p/P, exponent base: 2)
 * anything else         → base 10  (exponent char: e/E, exponent base: 10)
 *
 * On return *prefix_len is the number of characters consumed by the prefix
 * (0 for decimal, 2 for all others).
 *
 * exp_base is the base used for the exponent scaling:
 *   decimal / octal: exp_base == radix   (10 or 8)
 *   hex / binary:    exp_base == 2       (p-exponent is always a power of 2)
 */
static void detect_base(const char *str, int *radix, int *exp_base,
                         char *exp_char_lo, char *exp_char_hi,
                         size_t *prefix_len) {
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
    *radix        = 16;
    *exp_base     = 2;
    *exp_char_lo  = 'p';
    *exp_char_hi  = 'P';
    *prefix_len   = 2;
  } else if (str[0] == '0' && (str[1] == 'o' || str[1] == 'O')) {
    *radix        = 8;
    *exp_base     = 8;
    *exp_char_lo  = 'e';
    *exp_char_hi  = 'E';
    *prefix_len   = 2;
  } else if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
    *radix        = 2;
    *exp_base     = 2;
    *exp_char_lo  = 'p';
    *exp_char_hi  = 'P';
    *prefix_len   = 2;
  } else {
    *radix        = 10;
    *exp_base     = 10;
    *exp_char_lo  = 'e';
    *exp_char_hi  = 'E';
    *prefix_len   = 0;
  }
}

/* Return non-zero if c is a valid digit in the given radix. */
static int is_digit_in_base(unsigned char c, int radix) {
  if (radix <= 10)
    return (c >= '0' && c < '0' + radix);
  /* base 16 */
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int parse_exponent(const char *exp_str, long *exp_val) {
  char *endptr;
  long val = strtol(exp_str, &endptr, 10);
  if (*endptr != '\0') {
    /* exponent contains invalid chars or decimal point → reject */
    return -1;
  }
  *exp_val = val;
  return 0;
}

/*
 * Parse a numeric string (decimal, hex, octal, or binary) into an mpq_t.
 *
 * Supported formats (sign and leading whitespace are handled before the
 * prefix):
 *
 *   Decimal  :  [+-]  DIGITS [. DIGITS] [eE [+-] DIGITS]
 *   Hex      :  [+-] 0x HDIGITS [. HDIGITS] [pP [+-] DIGITS]
 *   Octal    :  [+-] 0o ODIGITS [. ODIGITS] [eE [+-] DIGITS]
 *   Binary   :  [+-] 0b BDIGITS [. BDIGITS] [pP [+-] DIGITS]
 *
 * For hex and binary the p-exponent scales by powers of 2 (matching C99
 * hex-float convention).  For octal the e-exponent scales by powers of 8.
 * For decimal the e-exponent scales by powers of 10.
 *
 * Returns 0 on success, -1 on any parse error.
 */
int mpq_set_decimal_str_exp(mpq_t r, const char *str, size_t len) {
  /* Skip leading whitespace */
  while (isspace((unsigned char)*str)) {
    str++;
    if (len > 0) len--;
  }

  /* Handle sign */
  int negative = 0;
  if (*str == '-') {
    negative = 1;
    str++; len--;
  } else if (*str == '+') {
    str++; len--;
  }

  /* Detect base and consume prefix */
  int radix, exp_base;
  char exp_char_lo, exp_char_hi;
  size_t prefix_len;
  detect_base(str, &radix, &exp_base, &exp_char_lo, &exp_char_hi, &prefix_len);
  str += prefix_len;
  if (len >= prefix_len) len -= prefix_len; else len = 0;

  /* Copy remaining input into a mutable buffer */
  char *buf = malloc(len + 1);
  if (!buf)
    return -1;
  memcpy(buf, str, len);
  buf[len] = '\0';

  /* Split off the exponent portion at the exponent character.
   * Note: for hex, exp_char_lo/hi are 'p'/'P', so hex digits a-f are never
   * mistaken for the exponent marker. */
  char *e_ptr = strchr(buf, exp_char_lo);
  if (!e_ptr)
    e_ptr = strchr(buf, exp_char_hi);

  char *exp_part = NULL;
  if (e_ptr) {
    *e_ptr   = '\0';
    exp_part = e_ptr + 1;
  }

  /* Validate mantissa: only radix-appropriate digits and at most one dot */
  int dot_count = 0;
  for (char *p = buf; *p; p++) {
    if (*p == '.') {
      if (++dot_count > 1) { free(buf); return -1; }
      continue;
    }
    if (!is_digit_in_base((unsigned char)*p, radix)) {
      free(buf); return -1;
    }
  }

  /* Split mantissa into integer and fractional parts */
  char *dot     = strchr(buf, '.');
  size_t int_len  = dot ? (size_t)(dot - buf) : strlen(buf);
  size_t frac_len = dot ? strlen(dot + 1)      : 0;

  /* Parse exponent (always written in decimal regardless of number base) */
  long exp_val = 0;
  if (exp_part) {
    while (isspace((unsigned char)*exp_part))
      exp_part++;

    int exp_neg = 0;
    if (*exp_part == '-') { exp_neg = 1; exp_part++; }
    else if (*exp_part == '+') { exp_part++; }

    if (!isdigit((unsigned char)*exp_part)) { free(buf); return -1; }

    char *endptr;
    exp_val = strtol(exp_part, &endptr, 10);
    if (*endptr != '\0' || endptr - exp_part > 11) { free(buf); return -1; }
    if (exp_neg) exp_val = -exp_val;
  }

  /* Build the combined digit string (int part || frac part, no dot) */
  size_t num_len = int_len + frac_len;
  if (num_len == 0) { free(buf); return -1; }

  char *num_str = malloc(num_len + 1);
  if (!num_str) { free(buf); return -1; }

  if (int_len  > 0) memcpy(num_str,           buf,     int_len);
  if (frac_len > 0) memcpy(num_str + int_len, dot + 1, frac_len);
  num_str[num_len] = '\0';

  /* ------------------------------------------------------------------
   * Build the exact rational value.
   *
   * The combined digit string represents the integer N (in `radix`).
   * The true value before applying the exponent is:
   *
   *   N / radix^frac_len
   *
   * After applying the exponent e (scale by exp_base^e):
   *
   *   value = N * exp_base^e / radix^frac_len
   *
   * Rearranged into numerator / denominator:
   *   if (frac_len_in_exp_base_bits - e) >= 0:  denom = exp_base^denom_exp
   *   otherwise:                                 numer = N * exp_base^|denom_exp|
   *
   * For decimal / octal (radix == exp_base):
   *   denom_exp = frac_len - exp_val
   *
   * For hex (radix=16, exp_base=2):
   *   Each fractional hex digit is worth 4 bits, so:
   *   denom_exp = 4 * frac_len - exp_val
   *
   * For binary (radix=2, exp_base=2):
   *   denom_exp = frac_len - exp_val
   * ------------------------------------------------------------------ */
  mpz_t numerator, denominator;
  mpz_init(numerator);
  mpz_init(denominator);

  if (mpz_set_str(numerator, num_str, radix) != 0) {
    free(num_str); free(buf);
    mpz_clear(numerator); mpz_clear(denominator);
    return -1;
  }
  free(num_str);
  free(buf);

  long denom_exp;
  if (radix == 16) {
    /* 1 hex digit == 4 bits == 4 powers of 2 */
    denom_exp = 4 * (long)frac_len - exp_val;
  } else {
    /* decimal, octal, binary: exponent base equals radix (or both are 2) */
    denom_exp = (long)frac_len - exp_val;
  }

  if (denom_exp >= 0) {
    mpz_ui_pow_ui(denominator, (unsigned long)exp_base, (unsigned long)denom_exp);
  } else {
    mpz_set_ui(denominator, 1);
    mpz_t scale;
    mpz_init(scale);
    mpz_ui_pow_ui(scale, (unsigned long)exp_base, (unsigned long)(-denom_exp));
    mpz_mul(numerator, numerator, scale);
    mpz_clear(scale);
  }

  mpq_set_num(r, numerator);
  mpq_set_den(r, denominator);
  mpq_canonicalize(r);

  if (negative)
    mpq_neg(r, r);

  mpz_clear(numerator);
  mpz_clear(denominator);
  return 0;
}

ParsedValueReturn parse_number(Token *token, char *path) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_NUMBER;
  mpq_t *r_ptr = malloc(sizeof(mpq_t));
  mpq_init(*r_ptr);
  int err = mpq_set_decimal_str_exp(*r_ptr, token->value, token->length);
  if (err) {
    mpq_clear(*r_ptr);
    free(r_ptr);
    free(parsedValue);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column,
                                          token->length, path, SyntaxError,
                                          "Unable to parse number"), NULL};
  }
  parsedValue->data = r_ptr;
  return (ParsedValueReturn){no_err, parsedValue};
}