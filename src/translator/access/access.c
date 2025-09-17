/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "access.h"

size_t translate_access(Translated *translated, ParsedAccess *access,
                        ArErr *err) {
  set_registers(translated, 1);
  uint64_t first = translate_parsed(translated, access->to_access, err);
  if (err->exists)
    return 0;
  push_instruction_byte(translated, OP_LOAD_GETATTRIBUTE_METHOD);
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, 1);
  
  translate_parsed(translated, access->access, err);
  if (err->exists)
    return 0;
  push_instruction_byte(translated, OP_INSERT_ARG);
  push_instruction_code(translated, 0);

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, access->line);
  push_instruction_code(translated, access->column);
  push_instruction_code(translated, access->length);
  push_instruction_byte(translated, OP_CALL);
  return first;
}