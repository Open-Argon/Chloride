/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../../hash_data/hash_data.h"
#include "../translator.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_string(Translated *translated,
                               ParsedString parsedString) {
  size_t string_pos = arena_push(&translated->constants, parsedString.string,
                                 parsedString.length);
  set_registers(translated, 1);
  size_t start = push_instruction_byte(translated, OP_LOAD_STRING);
  push_instruction_byte(translated, 0);
  push_instruction_code(translated, parsedString.length);
  push_instruction_code(translated, string_pos);
  push_instruction_code(translated, siphash64_bytes(parsedString.string,
                                                    parsedString.length,
                                                    siphash_key_fixed));
  return start;
}