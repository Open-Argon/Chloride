/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "for.h"
#include "../../hash_data/hash_data.h"
#include "../translator.h"
#include <stddef.h>
#include <string.h>

size_t translate_parsed_for(Translated *translated, ParsedFor *parsedFor,
                            ArErr *err) {
  uint8_t iterator_next_register = translated->registerAssignment++;
  set_registers(translated, translated->registerAssignment);
  struct break_or_return_jump old_break_jump = translated->break_jump;
  DArray break_jumps;
  darray_init(&break_jumps, sizeof(size_t));
  translated->break_jump.positions = &break_jumps;
  translated->break_jump.scope_depth = translated->scope_depth;
  size_t first = push_instruction_byte(translated, OP_NEW_SCOPE);
  translated->scope_depth++;
  translate_parsed(translated, parsedFor->iterator, err);
  if (err->exists) {
    return 0;
  }
  push_instruction_byte(translated, OP_LOAD_ITER_METHOD);

  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, 0);

  push_instruction_byte(translated, OP_CALL);

  push_instruction_byte(translated, OP_LOAD_NEXT_METHOD);

  push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, 0);
  push_instruction_byte(translated, iterator_next_register);

  size_t start_of_loop = push_instruction_byte(translated, OP_COPY_TO_REGISTER);
  push_instruction_byte(translated, iterator_next_register);
  push_instruction_byte(translated, 0);

  push_instruction_byte(translated, OP_INIT_CALL);
  push_instruction_code(translated, 0);

  push_instruction_byte(translated, OP_CALL);

  size_t length = strlen(parsedFor->key);
  size_t identifier_pos =
      arena_push(&translated->constants, parsedFor->key, length);
  push_instruction_byte(translated, OP_DECLARE);
  push_instruction_code(translated, length);
  push_instruction_code(translated, identifier_pos);
  push_instruction_code(translated,
                        siphash64_bytes(parsedFor->key, length,
                                        siphash_key_fixed_for_translator));
  push_instruction_byte(translated, 0);

  push_instruction_byte(translated, OP_IS_NOT_END_ITERATION);

  struct continue_jump old_continue_jump = translated->continue_jump;
  translated->continue_jump =
      (struct continue_jump){start_of_loop, translated->scope_depth};

  push_instruction_byte(translated, OP_JUMP_IF_FALSE);
  push_instruction_byte(translated, 0);
  uint64_t jump_index = push_instruction_code(translated, 0);

  translate_parsed(translated, parsedFor->content, err);

  push_instruction_byte(translated, OP_EMPTY_SCOPE);

  push_instruction_byte(translated, OP_JUMP);
  push_instruction_code(translated, start_of_loop);
  set_instruction_code(translated, jump_index, push_instruction_byte(translated, OP_POP_SCOPE));

  for (size_t i = 0; i < break_jumps.size; i++) {
    size_t *index = darray_get(&break_jumps, i);
    set_instruction_code(translated, *index, translated->bytecode.size);
  }
  darray_free(&break_jumps, NULL);
  translated->break_jump = old_break_jump;

  translated->continue_jump = old_continue_jump;
  translated->scope_depth--;
  translated->registerAssignment--;
  return first;
}