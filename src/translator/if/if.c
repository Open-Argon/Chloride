/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../../parser/if/if.h"
#include "if.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

size_t translate_parsed_if(Translated *translated, DArray *parsedIf, ArErr * err) {
  size_t *jump_after_body_positions =
      checked_malloc(parsedIf->size * sizeof(size_t));

  size_t first = translated->bytecode.size;

  set_registers(translated, 1);

  for (uint64_t i = 0; i < parsedIf->size; i++) {
    ParsedConditional *condition = darray_get(parsedIf, i);
    push_instruction_byte(translated, OP_NEW_SCOPE);
    if (condition->condition) {
      translate_parsed(translated, condition->condition, err);
      push_instruction_byte(translated, OP_BOOL);
      push_instruction_byte(translated, 0);
      push_instruction_byte(translated, OP_JUMP_IF_FALSE);
      push_instruction_byte(translated, 0);
      uint64_t last_jump_index = push_instruction_code(translated, 0);
      translate_parsed(translated, condition->content, err);
      push_instruction_byte(translated, OP_POP_SCOPE);
      push_instruction_byte(translated, OP_JUMP);
      jump_after_body_positions[i] = push_instruction_code(translated, 0);
      set_instruction_code(translated, last_jump_index,
                           translated->bytecode.size);
      push_instruction_byte(translated, OP_POP_SCOPE);
    } else {
      translate_parsed(translated, condition->content, err);
      push_instruction_byte(translated, OP_POP_SCOPE);
      push_instruction_byte(translated, OP_JUMP);
      jump_after_body_positions[i] = push_instruction_code(translated, 0);
    }
  }
  for (uint64_t i = 0; i < parsedIf->size; i++) {
    set_instruction_code(translated, jump_after_body_positions[i],
                           translated->bytecode.size);
  }
  free(jump_after_body_positions);
  return first;
}