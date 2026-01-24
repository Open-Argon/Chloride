/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "return.h"
#include "../../err.h"

size_t translate_parsed_return(Translated *translated,
                               ParsedReturn *parsedReturn, ArErr * err) {
  if (!translated->return_jumps) {
    *err = create_err(parsedReturn->line, parsedReturn->column, parsedReturn->length, translated->path, "Syntax Error", "nowhere to return to");
    return 0;
  }
  size_t first = translate_parsed(translated, parsedReturn->value, err);
  push_instruction_byte(translated, OP_JUMP);
  size_t return_up = push_instruction_code(translated, 0);
  darray_push(translated->return_jumps, &return_up);
  return first;
}