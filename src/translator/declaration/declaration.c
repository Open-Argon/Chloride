/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../parser/declaration/declaration.h"
#include "../translator.h"
#include "../../hash_data/hash_data.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
size_t translate_parsed_declaration(Translated *translated,
                                    DArray delcarations) {
  set_registers(translated, 1);
  size_t first = 0;
  for (size_t i = 0; i < delcarations.size; i++) {
    DArray* old_return_jumps = translated->return_jumps;
    translated->return_jumps = NULL;
    ParsedSingleDeclaration *singleDeclaration = darray_get(&delcarations, i);
    size_t temp = translate_parsed(translated, singleDeclaration->from);
    if (i == 0)
      first = temp;
    size_t length = strlen(singleDeclaration->name);
    size_t offset =
        arena_push(&translated->constants, singleDeclaration->name, length);

    push_instruction_byte(translated, OP_DECLARE);
    push_instruction_code(translated, length);
    push_instruction_code(translated, offset);
    push_instruction_code(translated, siphash64_bytes(singleDeclaration->name, length, siphash_key_fixed_for_translator));
    push_instruction_byte(translated, 0);
    SourceLocation source_locations = {singleDeclaration->line,
                                       singleDeclaration->column, length};
    push_instruction_code(translated, translated->source_locations.size);
    darray_push(&translated->source_locations, &source_locations);
    translated->return_jumps = old_return_jumps;
  }
  return first;
}