#include "runtime.h"
#include "../err.h"
#include "../hash_data/hash_data.h"
#include "../translator/translator.h"
#include "objects/functions/functions.h"
#include "objects/null/null.h"
#include "objects/object.h"
#include "objects/string/string.h"
#include "objects/type/type.h"
#include <fcntl.h>
#include <gc/gc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint64_t bytes_to_uint64(const uint8_t bytes[8]) {
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= ((uint64_t)bytes[i]) << (i * 8);
  }
  return value;
}

void init_types() {
  BASE_CLASS = init_argon_class("BASE_CLASS");

  init_type();
  init_function_type();
  init_null();
  init_string_type();

  init_base_field();
}

uint8_t pop_byte(Translated *translated, RuntimeState *state) {
  return *((uint8_t *)darray_get(&translated->bytecode, state->head++));
}

uint64_t pop_bytecode(Translated *translated, RuntimeState *state) {
  uint8_t bytes[8];
  for (size_t i = 0; i < sizeof(bytes); i++) {
    bytes[i] = *((uint8_t *)darray_get(&translated->bytecode, state->head++));
  }
  return bytes_to_uint64(bytes);
}

void load_const(Translated *translated, RuntimeState *state) {
  uint64_t to_register = pop_byte(translated, state);
  types type = pop_byte(translated, state);
  size_t length = pop_bytecode(translated, state);
  uint64_t offset = pop_bytecode(translated, state);

  void *data = ar_alloc_atomic(length);
  memcpy(data, arena_get(&translated->constants, offset), length);
  ArgonObject *object = ARGON_NULL;
  switch (type) {
  case TYPE_OP_STRING:
    object = init_string_object(data, length);
    break;
  }
  state->registers[to_register] = object;
}

ArErr load_variable(Translated *translated, RuntimeState *state,
                    struct Stack stack) {
  int64_t length = pop_bytecode(translated, state);
  int64_t offset = pop_bytecode(translated, state);
  int64_t source_location_index = pop_bytecode(translated, state);
  char *name = checked_malloc(length);
  memcpy(name, arena_get(&translated->constants, offset), length);
  uint64_t hash = siphash64_bytes(name, length, siphash_key);
  struct Stack *current_stack = &stack;
  while (current_stack) {
    ArgonObject *result = hashmap_lookup_GC(current_stack->scope, hash);
    if (result) {
      state->registers[0] = result;
      free(name);
      return no_err;
    }
    current_stack = current_stack->prev;
  }
  SourceLocation *source_location = darray_get(&translated->source_locations, source_location_index);
  ArErr err = create_err(source_location->line, source_location->column, length, state->path, "Name Error", "name '%.*s' is not defined",
                    (int)length, name);
  free(name);
  return err;
}

ArErr run_instruction(Translated *translated, RuntimeState *state,
                      struct Stack stack) {
  OperationType opcode = pop_byte(translated, state);
  switch (opcode) {
  case OP_LOAD_NULL:
    state->registers[pop_byte(translated, state)] = ARGON_NULL;
    break;
  case OP_LOAD_CONST:
    load_const(translated, state);
    break;
  case OP_LOAD_FUNCTION:
    load_argon_function(translated, state, stack);
    break;
  case OP_IDENTIFIER:
    return load_variable(translated, state, stack);
  default:
    return create_err(0, 0, 0, NULL, "Runtime Error", "Invalid Opcode %#x",
                      opcode);
  }
  return no_err;
}

RuntimeState init_runtime_state(Translated translated, char *path) {
  return (RuntimeState){
      checked_malloc(translated.registerCount * sizeof(ArgonObject *)), 0,
      path};
}

Stack create_scope(Stack *prev) {
  struct hashmap_GC *scope = createHashmap_GC();
  return (Stack){scope, prev};
}

ArErr runtime(Translated translated, RuntimeState state, Stack stack) {
  state.head = 0;
  while (state.head < translated.bytecode.size) {
    ArErr err = run_instruction(&translated, &state, stack);
    if (err.exists) {
      return err;
    }
  }
  return no_err;
}