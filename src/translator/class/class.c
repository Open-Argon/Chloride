/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "class.h"
#include "../translator.h"
#include "../../hash_data/hash_data.h"
#include <string.h>

size_t translate_parsed_class(Translated *translated, ParsedClass *parsedClass,
                              ArErr *err) {
  uint8_t parentRegister = translated->registerAssignment++;
  set_registers(translated, translated->registerAssignment);

  size_t length = strlen(parsedClass->name);
  size_t identifier_pos =
      arena_push(&translated->constants, parsedClass->name, length);

  char *init_object_name = "this";
  size_t init_object_name_length = strlen(init_object_name);
  size_t init_object_name_pos =
      arena_push(&translated->constants, init_object_name, init_object_name_length);
  
  char *class_parent = "super";
  size_t class_parent_length = strlen(class_parent);
  size_t class_parent_pos =
      arena_push(&translated->constants, class_parent, class_parent_length);

  size_t first;
  if (parsedClass->parent) {
    first = translate_parsed(translated, parsedClass->parent, err);
  } else {
    first = push_instruction_byte(translated, OP_LOAD_BASE_CLASS);
  }

  if (err->exists)
    return 0;

  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, parentRegister);

  push_instruction_byte(translated, OP_SOURCE_LOCATION);
  push_instruction_code(translated, parsedClass->line);
  push_instruction_code(translated, parsedClass->column);
  push_instruction_code(translated, length);

  push_instruction_byte(translated, OP_CREATE_CLASS);
  push_instruction_code(translated, length);
  push_instruction_code(translated, identifier_pos);

  push_instruction_byte(translated, OP_DECLARE);
  push_instruction_code(translated, length);
  push_instruction_code(translated, identifier_pos);
  push_instruction_code(translated,
                        siphash64_bytes(parsedClass->name, length,
                                        siphash_key_fixed_for_translator));
  push_instruction_byte(translated, 0);

  DArray return_jumps;
  push_instruction_byte(translated, OP_NEW_SCOPE);
  darray_init(&return_jumps, sizeof(size_t));
  DArray *old_return_jumps = translated->return_jumps;
  translated->return_jumps = &return_jumps;

  push_instruction_byte(translated, OP_DECLARE);
  push_instruction_code(translated, class_parent_length);
  push_instruction_code(translated, class_parent_pos);
  push_instruction_code(
      translated, siphash64_bytes(class_parent, class_parent_length,
                                  siphash_key_fixed_for_translator));
  push_instruction_byte(translated, parentRegister);
  
  push_instruction_byte(translated, OP_LOAD_NULL);
  push_instruction_byte(translated, parentRegister);

  push_instruction_byte(translated, OP_DECLARE);
  push_instruction_code(translated, init_object_name_length);
  push_instruction_code(translated, init_object_name_pos);
  push_instruction_code(
      translated, siphash64_bytes(init_object_name, init_object_name_length,
                                  siphash_key_fixed_for_translator));
  push_instruction_byte(translated, 0);

  translate_parsed(translated, parsedClass->body, err);

  if (err->exists)
    return 0;

  if (!old_return_jumps) {
    size_t return_jump_to = push_instruction_byte(translated, OP_POP_SCOPE);
    for (size_t i = 0; i < return_jumps.size; i++) {
      size_t *index = darray_get(&return_jumps, i);
      set_instruction_code(translated, *index, return_jump_to);
    }
  } else {
    push_instruction_byte(translated, OP_POP_SCOPE);
    push_instruction_byte(translated, OP_JUMP);
    size_t not_return_jump = push_instruction_code(translated, 0);
    size_t return_jump_to = push_instruction_byte(translated, OP_POP_SCOPE);
    push_instruction_byte(translated, OP_JUMP);
    size_t return_up = push_instruction_code(translated, 0);
    darray_push(old_return_jumps, &return_up);
    for (size_t i = 0; i < return_jumps.size; i++) {
      size_t *index = darray_get(&return_jumps, i);
      set_instruction_code(translated, *index, return_jump_to);
    }
    set_instruction_code(translated, not_return_jump,
                         translated->bytecode.size);
  }
  darray_free(&return_jumps, NULL);
  translated->return_jumps = old_return_jumps;
  translated->registerAssignment--;
  return first;
}