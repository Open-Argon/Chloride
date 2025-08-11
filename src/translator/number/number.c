/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "number.h"
#include "../translator.h"
#include <gmp.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_number(Translated *translated, mpq_t *number,
                               size_t to_register) {
  mpz_t num, den;
  mpz_init(num);
  mpz_init(den);
  mpz_set(num, mpq_numref(*number));
  mpz_set(den, mpq_denref(*number));
  size_t num_size;
  void *num_data = mpz_export(NULL, &num_size, 1, 1, 0, 0, num);
  size_t numerator_pos = arena_push(&translated->constants, num_data, num_size);
  free(num_data);
  mpz_clear(num);
  // Export denominator
  size_t den_size;
  void *den_data = mpz_export(NULL, &den_size, 1, 1, 0, 0, den);
  size_t denominator_pos =
      arena_push(&translated->constants, den_data, den_size);
  free(den_data);
  mpz_clear(den);
  set_registers(translated, to_register + 1);

  size_t start = push_instruction_byte(translated, OP_LOAD_NUMBER);
  push_instruction_byte(translated, to_register);
  push_instruction_code(translated, num_size);
  push_instruction_code(translated, numerator_pos);
  push_instruction_code(translated, den_size);
  push_instruction_code(translated, denominator_pos);
  return start;
}