/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../err.h"

void runtime_declaration(Translated *translated, RuntimeState *state,
                         struct Stack *stack, ArErr *err) {
  int64_t length = pop_bytecode(translated, state);
  int64_t offset = pop_bytecode(translated, state);
  int64_t prehash = pop_bytecode(translated, state);
  int64_t from_register = pop_byte(translated, state);
  void *data = arena_get(&translated->constants, offset);
  uint64_t hash = runtime_hash(data, length, prehash);
  ArgonObject *exists = hashmap_lookup_GC(stack->scope, hash);
  if (exists) {
    *err = create_err(
        state->source_location.line, state->source_location.column,
        state->source_location.length, state->path, "Runtime Error",
        "Identifier '%.*s' has already been declared in the current scope",
        length, arena_get(&translated->constants, offset));
  }
  ArgonObject *key = new_string_object(data, length, prehash, hash);
  hashmap_insert_GC(stack->scope, hash, key, state->registers[from_register],
                    0);
}