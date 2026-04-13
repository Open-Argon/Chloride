/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "item_access.h"
#include <stddef.h>
#include <stdio.h>

size_t translate_item_access(Translated *translated, ParsedItemAccess *access,
                             ArErr *err) {
  set_registers(translated, 1);
  uint64_t first = translate_parsed(translated, access->to_access, err);
  if (is_error(err))
    return 0;
  push_instruction_byte(translated, OP_LOAD_GETITEM_METHOD);
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, 1);
  if (access->subscripts.size != 1) {
    push_instruction_byte(translated, OP_LOAD_CREATE_TUPLE);
    push_instruction_byte(translated, OP_INIT_CALL);
    push_instruction_code(translated, access->subscripts.size);
  }
  for (size_t i = 0; i < access->subscripts.size; i++) {
    DArray *subscript = darray_get(&access->subscripts, i);
    if (subscript->size != 1) {
      push_instruction_byte(translated, OP_LOAD_SLICE_CLASS);
      push_instruction_byte(translated, OP_INIT_CALL);
      push_instruction_code(translated, subscript->size);
    }
    for (size_t j = 0; j < subscript->size; j++) {
      ParsedValue *item = *(ParsedValue **)darray_get(subscript, j);
      translate_parsed(translated, item, err);
      if (is_error(err))
        return 0;
      if (subscript->size != 1) {
        push_instruction_byte(translated, OP_INSERT_ARG);
        push_instruction_code(translated, j);
      }
    }
    if (subscript->size != 1) {
      push_instruction_byte(translated, OP_CALL);
    }
    if (access->subscripts.size != 1) {
      push_instruction_byte(translated, OP_INSERT_ARG);
      push_instruction_code(translated, i);
    }
  }
  if (access->subscripts.size != 1) {
    push_instruction_byte(translated, OP_CALL);
  }
  push_instruction_byte(translated, OP_INSERT_ARG);
  push_instruction_code(translated, 0);

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, access->line);
  push_instruction_code(translated, access->column);
  push_instruction_code(translated, access->length);
  push_instruction_byte(translated, OP_CALL);
  return first;
}