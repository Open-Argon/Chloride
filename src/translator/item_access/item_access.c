/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "item_access.h"
#include <stddef.h>

size_t translate_item_access(Translated *translated, ParsedItemAccess *access,
                        ArErr *err) {
  set_registers(translated, 1);
  uint64_t first = translate_parsed(translated, access->to_access, err);
  if (err->exists)
    return 0;
  push_instruction_byte(translated, OP_LOAD_GETITEM_METHOD);
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, access->itemc);

  for (size_t i = 0; i < access->itemc; i++) {
    translate_parsed(translated, access->items[i], err);
    if (err->exists)
      return 0;
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, i);
  }

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, access->line);
  push_instruction_code(translated, access->column);
  push_instruction_code(translated, access->length);
  push_instruction_byte(translated, OP_CALL);
  return first;
}