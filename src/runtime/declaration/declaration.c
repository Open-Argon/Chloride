/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"

ArErr runtime_declaration(Translated *translated, RuntimeState *state,
                          struct Stack *stack) {
  int64_t length = pop_bytecode(translated, state);
  int64_t offset = pop_bytecode(translated, state);
  int64_t prehash = pop_bytecode(translated, state);
  int64_t from_register = pop_byte(translated, state);
  int64_t source_location_index = pop_bytecode(translated, state);
  uint64_t hash = runtime_hash(arena_get(&translated->constants, offset), length, prehash);
  ArgonObject * exists = hashmap_lookup_GC(stack->scope, hash);
  if (exists) {
    SourceLocation *source_location = darray_get(&translated->source_locations, source_location_index);
    return create_err(source_location->line, source_location->column, source_location->length, state->path, "Runtime Error", "Identifier '%.*s' has already been declared in the current scope", length, arena_get(&translated->constants, offset));
  }
  ArgonObject * key = new_string_object(arena_get(&translated->constants, offset), length);
  hashmap_insert_GC(stack->scope, hash, key, state->registers[from_register], 0);
  return no_err;
}