/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "access.h"

size_t translate_access(Translated *translated, ParsedAccess *access,
                        ArErr *err) {
  set_registers(translated, 1);
  uint64_t first = translate_parsed(translated, &access->to_access, err);
  if (err->exists)
    return 0;
  push_instruction_byte(translated, OP_LOAD_GETATTRIBUTE_FUNCTION);
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, access->access.size+1);
  push_instruction_byte(translated, OP_LOAD_BOOL);
  push_instruction_byte(translated, access->access_fields);
  push_instruction_byte(translated, OP_INSERT_ARG);
  push_instruction_code(translated, 0);

  for (size_t i = 0; i < access->access.size; i++) {
    translate_parsed(translated, darray_get(&access->access, i), err);
    if (err->exists)
      return 0;
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, 1 + i);
  }

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, access->line);
  push_instruction_code(translated, access->column);
  push_instruction_code(translated, access->length);
  push_instruction_byte(translated, OP_CALL);
  return first;
}