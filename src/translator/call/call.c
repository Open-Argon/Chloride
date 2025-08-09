/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "call.h"

size_t translate_parsed_call(Translated *translated, ParsedCall *call,
                             ArErr *err) {
  set_registers(translated, 1);
  translate_parsed(translated, call->to_call, err);
  size_t first = push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, call->args.size);
  for (size_t i = 0; i < call->args.size; i++) {
    translate_parsed(translated, darray_get(&call->args, i), err);
    if (err->exists)
      return first;
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, i);
  }
  if (err->exists)
    return first;

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, call->line);
  push_instruction_code(translated, call->column);
  push_instruction_code(translated, 1);

  push_instruction_byte(translated, OP_CALL);
  return first;
}