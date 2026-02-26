/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../hash_data/hash_data.h"
#include "../../parser/declaration/declaration.h"
#include "../translator.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_declaration(Translated *translated, DArray delcarations,
                                    ArErr *err) {
  set_registers(translated, 1);
  size_t first = 0;
  for (size_t i = 0; i < delcarations.size; i++) {
    struct break_or_return_jump old_break_jump = translated->break_jump;
    translated->break_jump.positions = NULL;

    struct break_or_return_jump old_return_jump = translated->return_jump;
    translated->return_jump.positions = NULL;
    ParsedSingleDeclaration *singleDeclaration = darray_get(&delcarations, i);
    size_t temp = translate_parsed(translated, singleDeclaration->from, err);
    if (err->exists)
      return first;
    if (i == 0)
      first = temp;
    size_t length = strlen(singleDeclaration->name);
    size_t offset =
        arena_push(&translated->constants, singleDeclaration->name, length);

    push_instruction_byte(translated, OP_SOURCE_LOCATION);
    push_instruction_code(translated, singleDeclaration->line);
    push_instruction_code(translated, singleDeclaration->column);
    push_instruction_code(translated, length);

    push_instruction_byte(translated, OP_DECLARE);
    push_instruction_code(translated, length);
    push_instruction_code(translated, offset);
    push_instruction_code(translated,
                          siphash64_bytes(singleDeclaration->name, length,
                                          siphash_key_fixed_for_translator));
    push_instruction_byte(translated, 0);
    translated->return_jump = old_return_jump;
    translated->break_jump = old_break_jump;
  }
  return first;
}