/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "import.h"
#include "../../hash_data/hash_data.h"
#include "../translator.h"
#include <stddef.h>
#include <string.h>

size_t translate_parsed_import(Translated *translated,
                               ParsedImport *parsedImport, ArErr *err) {
  size_t first = translate_parsed(translated, parsedImport->file, err);

  if (err->exists)
    return 0;

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, parsedImport->line);
  push_instruction_code(translated, parsedImport->column);
  push_instruction_code(translated, parsedImport->length);
  push_instruction_byte(translated, OP_IMPORT);

  if (parsedImport->as) {
    size_t length = strlen(parsedImport->as);
    size_t as_pos =
        arena_push(&translated->constants, parsedImport->as, length);

    push_instruction_byte(translated, OP_DECLARE);
    push_instruction_code(translated, length);
    push_instruction_code(translated, as_pos);
    push_instruction_code(translated,
                          siphash64_bytes(parsedImport->as, length,
                                          siphash_key_fixed_for_translator));
    push_instruction_byte(translated, 0);
  }

  if (parsedImport->expose_all) {
    push_instruction_byte(translated, OP_EXPOSE_ALL);
  } else if (parsedImport->expose.resizable) {
    uint8_t registerA = 0;
    if (parsedImport->expose.size > 1) {
      registerA = translated->registerAssignment++;
      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, 0);
      push_instruction_byte(translated, registerA);
    }

    for (size_t i = 0; i < parsedImport->expose.size; i++) {
      if (i) {
        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, registerA);
        push_instruction_byte(translated, 0);
      }
      ParsedImportExpose *expose = darray_get(&parsedImport->expose, i);
      size_t length = strlen(expose->identifier);
      size_t pos =
          arena_push(&translated->constants, expose->identifier, length);

      push_instruction_byte(translated, OP_EXPOSE);
      push_instruction_code(translated, length);
      push_instruction_code(translated, pos);
      push_instruction_code(translated,
                            siphash64_bytes(expose->identifier, length,
                                            siphash_key_fixed_for_translator));

      if (expose->as) {
        length = strlen(expose->as);
        pos = arena_push(&translated->constants, expose->as, length);
      }

      push_instruction_byte(translated, OP_DECLARE);
      push_instruction_code(translated, length);
      push_instruction_code(translated, pos);
      push_instruction_code(translated,
                            siphash64_bytes(expose->as?expose->as:expose->identifier, length,
                                            siphash_key_fixed_for_translator));
      push_instruction_byte(translated, 0);
    }
    if (parsedImport->expose.size > 1) {
      translated->registerAssignment--;
    }
  }

  return first;
};
