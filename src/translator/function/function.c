/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "function.h"
#include "../../hash_data/hash_data.h"
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

  // A nested function body is compiled in total isolation from its
  // enclosing function: it gets its own bytecode buffer (below) and must
  // also get its own fresh scope/exception/jump-fixup bookkeeping.
  // Previously only `bytecode` and `registerAssignment` were saved and
  // restored here. `return_jump`, `break_jump`, `scope_depth`, and
  // `exception_handler_depth` were left pointing at the enclosing
  // function's (or enclosing do-block's) state. That meant a `return`
  // inside a nested function could record its jump-fixup position into
  // the *enclosing* function's return_jump.positions array. When the
  // enclosing function's do-block later backpatched those positions, it
  // wrote into whatever buffer was current at that time (the enclosing
  // function's own bytecode, since the nested function's buffer had
  // already been archived into the constant arena and freed) at an
  // offset that only made sense in the nested function's now-gone
  // buffer -- silently corrupting an unrelated instruction's operand
  // bytes in the enclosing function's bytecode.
  struct break_or_return_jump old_return_jump = translated->return_jump;
  struct break_or_return_jump old_break_jump = translated->break_jump;
  uint64_t old_scope_depth = translated->scope_depth;
  uint64_t old_exception_handler_depth = translated->exception_handler_depth;

  translated->return_jump = (struct break_or_return_jump){0};
  translated->break_jump = (struct break_or_return_jump){0};
  translated->scope_depth = 0;
  translated->exception_handler_depth = 0;

  translated->registerAssignment = 1;
  darray_init(&translated->bytecode, sizeof(uint8_t));
  set_registers(translated, 1);
  translate_parsed(translated, parsedFunction->body, err);
  size_t function_bytecode_offset =
      arena_push(&translated->constants, translated->bytecode.data,
                 translated->bytecode.size * translated->bytecode.element_size);
  size_t function_bytecode_length = translated->bytecode.size;
  darray_free(&translated->bytecode, NULL);
  translated->bytecode = main_bytecode;
  translated->registerAssignment = old_assignment;

  translated->return_jump = old_return_jump;
  translated->break_jump = old_break_jump;
  translated->scope_depth = old_scope_depth;
  translated->exception_handler_depth = old_exception_handler_depth;

  size_t start = push_instruction_byte(translated, OP_LOAD_FUNCTION);
  size_t offset = arena_push(&translated->constants, parsedFunction->name,
                             strlen(parsedFunction->name));
  push_instruction_code(translated, offset);
  push_instruction_code(translated, strlen(parsedFunction->name));
  push_instruction_code(translated, function_bytecode_offset);
  push_instruction_code(translated, function_bytecode_length *
                                        translated->bytecode.element_size);
  push_instruction_code(translated, parsedFunction->parameters.size);
  push_instruction_code(translated,
                        parsedFunction->default_value_parameters
                            ? parsedFunction->default_value_parameters->size
                            : 0);

  for (size_t i = 0; i < parsedFunction->parameters.size; i++) {
    char **parameter_name = darray_get(&parsedFunction->parameters, i);
    offset = arena_push(&translated->constants, *parameter_name,
                        strlen(*parameter_name));
    push_instruction_byte(translated, OP_SET_FUNCTION_PARAMETER);
    push_instruction_code(translated, i);
    push_instruction_code(translated, offset);
    push_instruction_code(translated, strlen(*parameter_name));
    push_instruction_code(translated, siphash64_bytes(*parameter_name,
                                                      strlen(*parameter_name),
                                                      siphash_key_fixed));
  }

  if (parsedFunction->v_parameter) {
    char *parameter_name = parsedFunction->v_parameter;
    offset = arena_push(&translated->constants, parameter_name,
                        strlen(parameter_name));
    push_instruction_byte(translated, OP_SET_FUNCTION_POSITIONAL_PARAMETER);
    push_instruction_code(translated, offset);
    push_instruction_code(translated, strlen(parameter_name));
    push_instruction_code(translated, siphash64_bytes(parameter_name,
                                                      strlen(parameter_name),
                                                      siphash_key_fixed));
  }

  if (parsedFunction->kw_parameter) {
    char *parameter_name = parsedFunction->kw_parameter;
    offset = arena_push(&translated->constants, parameter_name,
                        strlen(parameter_name));
    push_instruction_byte(translated, OP_SET_FUNCTION_KEY_WORD_PARAMETER);
    push_instruction_code(translated, offset);
    push_instruction_code(translated, strlen(parameter_name));
    push_instruction_code(translated, siphash64_bytes(parameter_name,
                                                      strlen(parameter_name),
                                                      siphash_key_fixed));
  }

  if (parsedFunction->default_value_parameters) {
    uint8_t funcRegister = translated->registerAssignment++;
    set_registers(translated, translated->registerAssignment);
    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, 0);
    push_instruction_byte(translated, funcRegister);
    for (size_t i = 0; i < parsedFunction->default_value_parameters->size;
         i++) {
      struct default_value_parameter *parameter =
          darray_get(parsedFunction->default_value_parameters, i);

      translate_parsed(translated, parameter->value, err);

      offset = arena_push(&translated->constants, parameter->name,
                          strlen(parameter->name));
      push_instruction_byte(translated, OP_SET_FUNCTION_DEFAULT_PARAMETER);
      push_instruction_byte(translated, funcRegister);
      push_instruction_code(translated, i);
      push_instruction_code(translated, offset);
      push_instruction_code(translated, strlen(parameter->name));
      push_instruction_code(translated, siphash64_bytes(parameter->name,
                                                        strlen(parameter->name),
                                                        siphash_key_fixed));
    }
    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, funcRegister);
    push_instruction_byte(translated, 0);
    translated->registerAssignment--;
  }
  return start;
}