/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "operation.h"
#include <stddef.h>

size_t translate_operation(Translated *translated, ParsedOperation *operation,
                           ArErr *err) {
  set_registers(translated, 1);
  uint64_t first;
  switch (operation->operation) {
  case TOKEN_PLUS:;
    first = push_instruction_byte(translated, OP_LOAD_ADDITION_FUNCTION);
    break;
  case TOKEN_MINUS:
    first = push_instruction_byte(translated, OP_LOAD_SUBTRACTION_FUNCTION);
    break;
  case TOKEN_STAR:
    first = push_instruction_byte(translated, OP_LOAD_MULTIPLY_FUNCTION);
    break;
  case TOKEN_SLASH:
    first = push_instruction_byte(translated, OP_LOAD_DIVISION_FUNCTION);
    break;
  default:
    *err = create_err(operation->line, operation->column, operation->length,
                      translated->path, "Syntax Error", "unknown operation");
    return 0;
  }
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, operation->to_operate_on.size);

  for (size_t i = 0; i < operation->to_operate_on.size; i++) {
    translate_parsed(translated, darray_get(&operation->to_operate_on, i), err);
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, i);
  }

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, operation->line);
  push_instruction_code(translated, operation->column);
  push_instruction_code(translated, operation->length);
  push_instruction_byte(translated, OP_CALL);
  return first;
}