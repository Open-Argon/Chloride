/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include "dowrap.h"
#include <stddef.h>

size_t translate_parsed_dowrap(Translated *translated, DArray *parsedDowrap) {
  set_registers(translated, 1);

  size_t first = translated->bytecode.size;

  if (parsedDowrap->size) {
    DArray return_jumps;
    push_instruction_byte(translated, OP_NEW_SCOPE);
    darray_init(&return_jumps, sizeof(size_t));
    DArray *old_return_jumps = translated->return_jumps;
    translated->return_jumps = &return_jumps;
    for (size_t i = 0; i < parsedDowrap->size; i++) {
      ParsedValue *parsedValue = darray_get(parsedDowrap, i);
      translate_parsed(translated, parsedValue);
    }
    push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, 0);
    if (!old_return_jumps) {
      size_t return_jump_to = push_instruction_byte(translated, OP_POP_SCOPE);
      for (size_t i = 0; i < return_jumps.size; i++) {
        size_t *index = darray_get(&return_jumps, i);
        set_instruction_code(translated, *index, return_jump_to);
      }
    } else {
      push_instruction_byte(translated, OP_POP_SCOPE);
      push_instruction_byte(translated, OP_JUMP);
      size_t not_return_jump = push_instruction_code(translated, 0);
      size_t return_jump_to = push_instruction_byte(translated, OP_POP_SCOPE);
      push_instruction_byte(translated, OP_JUMP);
      size_t return_up = push_instruction_code(translated, 0);
      darray_push(old_return_jumps, &return_up);
      for (size_t i = 0; i < return_jumps.size; i++) {
        size_t *index = darray_get(&return_jumps, i);
        set_instruction_code(translated, *index, return_jump_to);
      }
      set_instruction_code(translated, not_return_jump, translated->bytecode.size);
    }
    darray_free(&return_jumps, NULL);
    translated->return_jumps = old_return_jumps;
  } else {
    push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, 0);
  }
  return first;
}