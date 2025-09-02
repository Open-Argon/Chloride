/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "function.h"
#include "../translator.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
size_t translate_parsed_function(Translated *translated,
                                 ParsedFunction *parsedFunction, ArErr *err) {
  DArray main_bytecode = translated->bytecode;
  uint8_t old_assignment = translated->registerAssignment;
  translated->registerAssignment = 1;
  darray_init(&translated->bytecode, sizeof(uint8_t));
  set_registers(translated, 1);
  translate_parsed(translated, parsedFunction->body, err);
  size_t function_bytecode_offset =
      arena_push(&translated->constants, translated->bytecode.data,
                 translated->bytecode.size*translated->bytecode.element_size);
  size_t function_bytecode_length = translated->bytecode.size;
  darray_free(&translated->bytecode, NULL);
  translated->bytecode = main_bytecode;
  translated->registerAssignment = old_assignment;
  size_t start = push_instruction_byte(translated, OP_LOAD_FUNCTION);
  size_t offset = arena_push(&translated->constants, parsedFunction->name,
                             strlen(parsedFunction->name));
  push_instruction_code(translated, offset);
  push_instruction_code(translated, strlen(parsedFunction->name));
  push_instruction_code(translated, parsedFunction->parameters.size);
  for (size_t i = 0; i < parsedFunction->parameters.size; i++) {
    char **parameter_name = darray_get(&parsedFunction->parameters, i);
    offset = arena_push(&translated->constants, *parameter_name,
                        strlen(*parameter_name));
    push_instruction_code(translated, offset);
    push_instruction_code(translated, strlen(*parameter_name));
  }
  push_instruction_code(translated, function_bytecode_offset);
  push_instruction_code(translated, function_bytecode_length*translated->bytecode.element_size);
  return start;
}