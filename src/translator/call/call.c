/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "call.h"
#include "../../err.h"
#include "../../parser/function/function.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../translator.h"
#include <string.h>
#include "../../hash_data/hash_data.h"

size_t translate_parsed_call(Translated *translated, ParsedCall *call,
                             ArErr *err) {
  if (call->must_assign) {
    *err =
        path_specific_create_err(call->line, call->column, 1, translated->path,
                                 SyntaxError, "Invalid syntax");
    return 0;
  }
  set_registers(translated, 1);
  size_t first = translate_parsed(translated, call->to_call, err);
  if (is_error(err)) {
    return first;
  }
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, call->args.size);
  push_instruction_byte(translated, OP_NEW_SCOPE);
  translated->scope_depth++;

  struct break_or_return_jump old_break_jump = translated->break_jump;
  translated->break_jump.positions = NULL;

  struct break_or_return_jump old_return_jump = translated->return_jump;
  translated->return_jump.positions = NULL;
  for (size_t i = 0; i < call->args.size; i++) {
    translate_parsed(translated, darray_get(&call->args, i), err);
    if (is_error(err)) {
      translated->return_jump = old_return_jump;
      return first;
    }
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, i);
  }

  if (call->kwargs) {
    for (size_t i = 0; i < call->kwargs->size; i++) {
      struct default_value_parameter *arg =
          (struct default_value_parameter *)darray_get(call->kwargs, i);
      translate_parsed(translated, arg->value, err);
      if (is_error(err)) {
        translated->return_jump = old_return_jump;
        return first;
      }
      size_t length = strlen(arg->name);
      size_t identifier_pos =
          arena_push(&translated->constants, arg->name, length);
      set_registers(translated, 1);
      push_instruction_byte(translated, OP_SET_KEY_WORD_ARG);
      push_instruction_code(translated, length);
      push_instruction_code(translated, identifier_pos);
      push_instruction_code(
          translated,
          siphash64_bytes(arg->name, length, siphash_key_fixed));
    }
  }

  translated->return_jump = old_return_jump;
  translated->break_jump = old_break_jump;

  push_instruction_byte(translated, OP_POP_SCOPE);
  translated->scope_depth--;

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, call->line);
  push_instruction_code(translated, call->column);
  push_instruction_code(translated, 1);

  push_instruction_byte(translated, OP_CALL);
  return first;
}