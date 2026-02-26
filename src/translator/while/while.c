/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "while.h"
#include <stddef.h>

size_t translate_parsed_while(Translated *translated, ParsedWhile *parsedWhile,
                              ArErr *err) {
  set_registers(translated, 1);
  struct break_or_return_jump old_break_jump = translated->break_jump;
  DArray break_jumps;
  darray_init(&break_jumps, sizeof(size_t));
  translated->break_jump.positions = &break_jumps;
  translated->break_jump.scope_depth = translated->scope_depth;
  size_t first = push_instruction_byte(translated, OP_NEW_SCOPE);
  translated->scope_depth++;
  size_t start_of_loop =
      translate_parsed(translated, parsedWhile->condition, err);
  if (err->exists) {
    return 0;
  }
  struct continue_jump old_continue_jump = translated->continue_jump;
  translated->continue_jump =
      (struct continue_jump){start_of_loop, translated->scope_depth};

  push_instruction_byte(translated, OP_BOOL);
  push_instruction_byte(translated, OP_JUMP_IF_FALSE);
  push_instruction_byte(translated, 0);
  uint64_t jump_index = push_instruction_code(translated, 0);
  translate_parsed(translated, parsedWhile->content, err);
  push_instruction_byte(translated, OP_EMPTY_SCOPE);
  push_instruction_byte(translated, OP_JUMP);
  push_instruction_code(translated, start_of_loop);
  set_instruction_code(translated, jump_index, translated->bytecode.size);
  push_instruction_byte(translated, OP_POP_SCOPE);

  for (size_t i = 0; i < break_jumps.size; i++) {
    size_t *index = darray_get(&break_jumps, i);
    set_instruction_code(translated, *index, translated->bytecode.size);
  }
  darray_free(&break_jumps, NULL);
  translated->break_jump = old_break_jump;

  translated->continue_jump = old_continue_jump;
  translated->scope_depth--;
  return first;
}