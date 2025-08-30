/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "operation.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

size_t translate_operation(Translated *translated, ParsedOperation *operation,
                           ArErr *err) {
  uint8_t registerA = translated->registerAssignment++;
  uint8_t registerB = translated->registerAssignment++;
  set_registers(translated, translated->registerAssignment);
  uint64_t first = translate_parsed(translated, darray_get(&operation->to_operate_on, 0), err);
  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, registerA);
  for (size_t i = 1; i < operation->to_operate_on.size; i++) {
    translate_parsed(translated, darray_get(&operation->to_operate_on, i), err);
    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, 0);
    push_instruction_byte(translated, registerB);
    switch (operation->operation) {
    case TOKEN_PLUS:;
      push_instruction_byte(translated, OP_ADDITION);
      break;
    case TOKEN_MINUS:;
      push_instruction_byte(translated, OP_SUBTRACTION);
      break;
    default:
      *err = create_err(operation->line, operation->column, operation->length,
                        translated->path, "Syntax Error", "unknown operation");
      return 0;
    }
    push_instruction_byte(translated, registerA);
    push_instruction_byte(translated, registerB);
    push_instruction_byte(
        translated, operation->to_operate_on.size - 1 == i ? 0 : registerA);
  }
  push_instruction_byte(translated, OP_LOAD_NULL);
  push_instruction_byte(translated, registerA);
  push_instruction_byte(translated, OP_LOAD_NULL);
  push_instruction_byte(translated, registerB);
  translated->registerAssignment -= 2;
  return first;
}