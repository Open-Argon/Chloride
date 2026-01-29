/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../err.h"
#include <stdint.h>
#include "../objects/string/string.h"

void runtime_declaration(int64_t length,int64_t offset,int64_t prehash,uint8_t from_register,Translated *translated, RuntimeState *state,
                         struct Stack *stack, ArErr *err) {
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