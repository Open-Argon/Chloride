/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "break.h"
#include "../../err.h"
#include "../translator.h"

size_t translate_parsed_break(Translated *translated,
                              ParsedContinueOrBreak *parsedBreak, ArErr *err) {
  if (!translated->break_jump.positions) {
    *err =
        create_err(parsedBreak->line, parsedBreak->column, parsedBreak->length,
                   translated->path, "Syntax Error", "nowhere to break to");
    return 0;
  }
  size_t first;
  uint64_t i;
  for (i = 0;
       i < (translated->scope_depth - translated->break_jump.scope_depth);
       i++) {
    size_t pos = push_instruction_byte(translated, OP_POP_SCOPE);
    if (i == 0)
      first = pos;
  }
  size_t pos = push_instruction_byte(translated, OP_JUMP);
  if (i == 0)
    first = pos;
  size_t break_up = push_instruction_code(translated, 0);
  darray_push(translated->break_jump.positions, &break_up);
  return first;
}