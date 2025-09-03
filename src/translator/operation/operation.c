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
  if (operation->operation == TOKEN_AND) {
    size_t *jump_to_if_false =
        checked_malloc(operation->to_operate_on.size * sizeof(size_t));
    uint8_t registerA = translated->registerAssignment++;
    set_registers(translated, translated->registerAssignment);
    uint64_t first = 0;
    for (size_t i = 0; i < operation->to_operate_on.size; i++) {
      uint64_t position = translate_parsed(
          translated, darray_get(&operation->to_operate_on, i), err);
      if (i == 0)
        position = first;
      if (err->exists) {
        free(jump_to_if_false);
        return first;
      }
      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, 0);
      push_instruction_byte(translated, registerA);

      push_instruction_byte(translated, OP_BOOL);
      push_instruction_byte(translated, registerA);

      push_instruction_byte(translated, OP_JUMP_IF_FALSE);
      push_instruction_byte(translated, registerA);
      jump_to_if_false[i] = push_instruction_code(translated, 0);
    }
    for (size_t i = 0; i < operation->to_operate_on.size; i++) {
      set_instruction_code(translated, jump_to_if_false[i],
                           translated->bytecode.size);
    }

    free(jump_to_if_false);
    return first;
  }
  uint8_t registerA = translated->registerAssignment++;
  uint8_t registerB = translated->registerAssignment++;
  set_registers(translated, translated->registerAssignment);
  uint64_t first = translate_parsed(
      translated, darray_get(&operation->to_operate_on, 0), err);
  if (err->exists)
    return first;
  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, registerA);
  for (size_t i = 1; i < operation->to_operate_on.size; i++) {
    translate_parsed(translated, darray_get(&operation->to_operate_on, i), err);
    if (err->exists)
      return first;
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
    case TOKEN_STAR:;
      push_instruction_byte(translated, OP_MULTIPLICATION);
      break;
    case TOKEN_SLASH:;
      push_instruction_byte(translated, OP_DIVISION);
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