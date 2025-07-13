/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../translator.h"
#include "identifier.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../../hash_data/hash_data.h"

size_t translate_parsed_identifier(Translated *translated, ParsedIdentifier *parsedIdentifier) {
  size_t length = strlen(parsedIdentifier->name);
  size_t identifier_pos = arena_push(&translated->constants, parsedIdentifier->name, length);
  set_registers(translated, 1);
  size_t start = push_instruction_byte(translated, OP_IDENTIFIER);
  push_instruction_code(translated,length);
  push_instruction_code(translated, identifier_pos);
    push_instruction_code(translated, siphash64_bytes(parsedIdentifier->name, length, siphash_key_fixed_for_translator));
  SourceLocation source_locations = {
    parsedIdentifier->line,
    parsedIdentifier->column,
    length
  };
  push_instruction_code(translated, translated->source_locations.size);
  darray_push(&translated->source_locations, &source_locations);
  return start;
}