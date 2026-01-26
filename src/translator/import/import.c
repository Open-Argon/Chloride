/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "import.h"
#include "../translator.h"
#include "../../hash_data/hash_data.h"
#include <stddef.h>
#include <string.h>

size_t translate_parsed_import(Translated *translated,
                               ParsedImport *parsedImport, ArErr *err) {
  size_t first = translate_parsed(translated, parsedImport -> file, err);

  if (err->exists) return 0;

  size_t length = strlen(parsedImport->as);
  size_t as_pos =
      arena_push(&translated->constants, parsedImport->as, length);

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, parsedImport->line);
  push_instruction_code(translated, parsedImport->column);
  push_instruction_code(translated, parsedImport->length);
  push_instruction_byte(translated, OP_IMPORT);

    push_instruction_byte(translated, OP_DECLARE);
  push_instruction_code(translated, length);
  push_instruction_code(translated, as_pos);
  push_instruction_code(translated,
                        siphash64_bytes(parsedImport->as, length,
                                        siphash_key_fixed_for_translator));
  push_instruction_byte(translated, 0);

  return first;
};
