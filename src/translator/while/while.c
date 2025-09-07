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
  DArray return_jumps;
  DArray *old_return_jumps = NULL;
  if (translated->return_jumps) {
    darray_init(&return_jumps, sizeof(size_t));
    old_return_jumps = translated->return_jumps;
    translated->return_jumps = &return_jumps;
  }
  size_t first = push_instruction_byte(translated, OP_NEW_SCOPE);
  size_t start_of_loop =
      translate_parsed(translated, parsedWhile->condition, err);
  if (err->exists) {
    return 0;
  }
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
  if (translated->return_jumps) {
    push_instruction_byte(translated, OP_JUMP);
    size_t skip_return = push_instruction_code(translated, 0);

    size_t return_jump_to = push_instruction_byte(translated, OP_POP_SCOPE);
    push_instruction_byte(translated, OP_JUMP);
    size_t return_up = push_instruction_code(translated, 0);
    darray_push(old_return_jumps, &return_up);
    for (size_t i = 0; i < return_jumps.size; i++) {
      size_t *index = darray_get(&return_jumps, i);
      set_instruction_code(translated, *index, return_jump_to);
    }
    set_instruction_code(translated, skip_return, translated->bytecode.size);
    darray_free(&return_jumps, NULL);
    translated->return_jumps = old_return_jumps;
  }
  return first;
}