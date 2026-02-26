/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dowrap.h"
#include <stddef.h>

size_t translate_parsed_dowrap(Translated *translated, DArray *parsedDowrap,
                               ArErr *err) {
  set_registers(translated, 1);

  size_t first = translated->bytecode.size;

  if (parsedDowrap->size) {
    struct break_or_return_jump old_return_jump = translated->return_jump;
    DArray return_jumps;

    if (!old_return_jump.positions) {
      darray_init(&return_jumps, sizeof(size_t));
      translated->return_jump.positions = &return_jumps;
      translated->return_jump.scope_depth = translated->scope_depth;
    }
    translated->scope_depth++;
    push_instruction_byte(translated, OP_NEW_SCOPE);
    *err = translate(translated, parsedDowrap);
    if (err->exists)
      return first;
    push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, 0);
    push_instruction_byte(translated, OP_POP_SCOPE);
    if (!old_return_jump.positions) {
      for (size_t i = 0; i < return_jumps.size; i++) {
        size_t *index = darray_get(&return_jumps, i);
        set_instruction_code(translated, *index, translated->bytecode.size);
      }
      darray_free(&return_jumps, NULL);
      translated->return_jump = old_return_jump;
    }
    translated->scope_depth--;
  } else {
    push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, 0);
  }
  return first;
}