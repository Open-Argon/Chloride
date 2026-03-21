/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../../hash_data/hash_data.h"
#include "../translator.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_string(Translated *translated,
                               ParsedString parsedString) {
  size_t string_pos = arena_push(&translated->constants, parsedString.string,
                                 parsedString.length);
  set_registers(translated, 1);
  size_t start = push_instruction_byte(translated, OP_LOAD_STRING);
  push_instruction_byte(translated, 0);
  push_instruction_code(translated, parsedString.length);
  push_instruction_code(translated, string_pos);
  push_instruction_code(translated, siphash64_bytes(parsedString.string,
                                                    parsedString.length,
                                                    siphash_key_fixed));
  return start;
}

size_t translate_parsed_template(Translated *translated,
                                 ParsedTemplate *parsedTemplate, ArErr *err) {
  set_registers(translated, 1);
  size_t first;
  if (parsedTemplate->templater) {
    first = translate_parsed(translated, parsedTemplate->templater, err);
    if (is_error(err))
      return 0;
    push_instruction_byte(translated, OP_LOAD_TEMPLATE_METHOD);
  } else {
    first = push_instruction_byte(translated, OP_LOAD_TEMPLATE);
  }
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, 1);
  push_instruction_byte(translated, OP_NEW_SCOPE);
  translated->scope_depth++;
  push_instruction_byte(translated, OP_LOAD_CREATE_TUPLE);
  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, parsedTemplate->values.size);
  for (size_t i = 0; i < parsedTemplate->values.size; i++) {
    TemplateValue *item = darray_get(&parsedTemplate->values, i);
    push_instruction_byte(translated, OP_LOAD_CREATE_TUPLE);
    push_instruction_byte(translated, OP_INIT_CALL);
    push_instruction_code(translated, 2);
    push_instruction_byte(translated, OP_LOAD_BOOL);
    push_instruction_byte(translated, item->is_string);
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, 0);

    if (!item->is_string) {
      translate_parsed(translated, item->value.value, err);
      if (is_error(err))
        return 0;
    } else {
      size_t string_pos =
          arena_push(&translated->constants, item->value.string.string,
                     item->value.string.length);
      push_instruction_byte(translated, OP_LOAD_STRING);
      push_instruction_byte(translated, 0);
      push_instruction_code(translated, item->value.string.length);
      push_instruction_code(translated, string_pos);
      push_instruction_code(translated,
                            siphash64_bytes(item->value.string.string,
                                            item->value.string.length,
                                            siphash_key_fixed));
    }
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, 1);
    push_instruction_byte(translated, OP_CALL);
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, i);
  }
  push_instruction_byte(translated, OP_CALL);
  push_instruction_byte(translated, OP_INSERT_ARG);
  push_instruction_code(translated, 0);

  push_instruction_byte(translated, OP_CALL);

  push_instruction_byte(translated, OP_POP_SCOPE);
  translated->scope_depth--;
  return first;
}