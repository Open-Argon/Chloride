/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "continue.h"
#include "../../err.h"
#include "../translator.h"
#include <stddef.h>

size_t translate_parsed_continue(Translated *translated,
                                 ParsedContinue *parsedContinue, ArErr *err) {
  if (translated->continue_jump.pos == -1) {
    *err = create_err(parsedContinue->line, parsedContinue->column,
                      parsedContinue->length, translated->path, "Syntax Error",
                      "nowhere to continue to");
    return 0;
  }
  size_t first;
  uint64_t i;
  for (i = 0;
       i < (translated->scope_depth - translated->continue_jump.scope_depth);
       i++) {
    size_t pos = push_instruction_byte(translated, OP_POP_SCOPE);
    if (i == 0)
      first = pos;
  }
  size_t pos = push_instruction_byte(translated, OP_JUMP);
  if (i == 0)
    first = pos;
  push_instruction_code(translated, translated->continue_jump.pos);
  return first;
}