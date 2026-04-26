/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "try.h"
#include "../../hash_data/hash_data.h"
#include "../translator.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

size_t translate_parsed_try(Translated *translated, ParsedTry *parsedTry,
                            ArErr *err) {
  uint8_t err_register = translated->registerAssignment++;
  set_registers(translated, translated->registerAssignment);

  translated->exception_handler_depth++;
  size_t first = push_instruction_byte(translated, OP_EXCEPTION_CATCHER_PUSH);
  size_t exception_jump_pos = push_instruction_code(translated, 0);
  translated->scope_depth++;
  push_instruction_byte(translated, OP_NEW_SCOPE);

  translate_parsed(translated, parsedTry->try_body, err);
  if (is_error(err)) {
    return 0;
  }

  translated->scope_depth--;
  push_instruction_byte(translated, OP_POP_SCOPE);
  translated->exception_handler_depth--;
  push_instruction_byte(translated, OP_EXCEPTION_CATCHER_POP);

  push_instruction_byte(translated, OP_JUMP);
  size_t done_pos = push_instruction_code(translated, 0);

  translated->scope_depth++;
  set_instruction_code(translated, exception_jump_pos,
                       push_instruction_byte(translated, OP_NEW_SCOPE));

  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, err_register);

  push_instruction_byte(translated, OP_LOAD_IS_INSTANCE_FUNCTION);
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, 2);

  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, err_register);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, OP_INSERT_ARG);
  push_instruction_code(translated, 0);

  if (parsedTry->exception_type) {
    translate_parsed(translated, parsedTry->exception_type, err);
    if (is_error(err)) {
      return 0;
    }
  } else {
    push_instruction_byte(translated, OP_LOAD_EXCEPTION_CLASS);
  }
  push_instruction_byte(translated, OP_INSERT_ARG);
  push_instruction_code(translated, 1);

  push_instruction_byte(translated, OP_CALL);
  push_instruction_byte(translated, OP_NOT);

  push_instruction_byte(translated, OP_JUMP_IF_FALSE);
  push_instruction_byte(translated, 0);
  size_t is_exception_type = push_instruction_code(translated, 0);

  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, err_register);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, OP_THROW);

  set_instruction_code(translated, is_exception_type,
                       translated->bytecode.size);

  if (parsedTry->exception_name) {
    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, err_register);
    push_instruction_byte(translated, 0);
    size_t length = strlen(parsedTry->exception_name);
    size_t identifier_pos =
        arena_push(&translated->constants, parsedTry->exception_name, length);
    push_instruction_byte(translated, OP_DECLARE);
    push_instruction_code(translated, length);
    push_instruction_code(translated, identifier_pos);
    push_instruction_code(
        translated,
        siphash64_bytes(parsedTry->exception_name, length, siphash_key_fixed));
    push_instruction_byte(translated, 0);
  }

  translate_parsed(translated, parsedTry->catch_body, err);
  if (is_error(err)) {
    return 0;
  }
  translated->scope_depth--;
  push_instruction_byte(translated, OP_POP_SCOPE);

  set_instruction_code(translated, done_pos, translated->bytecode.size);

  translated->registerAssignment--;
  return first;
}