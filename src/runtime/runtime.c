#include "runtime.h"
#include "../translator/translator.h"
#include "internals/siphash/siphash.h"
#include "objects/null/null.h"
#include "objects/object.h"
#include "objects/string/string.h"
#include "objects/type/type.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void init_types() {
  BASE_OBJECT = init_argon_object();

  init_type();
  init_null();
  init_string_type();

  init_base_field();
}

uint64_t pop_bytecode(Translated *translated, RuntimeState *state) {
  uint64_t *instruction = darray_get(&translated->bytecode, state->head++);
  return *instruction;
}

void load_const(Translated *translated, RuntimeState *state) {
  uint64_t to_register = pop_bytecode(translated, state);
  types type = pop_bytecode(translated, state);
  size_t length = pop_bytecode(translated, state);
  uint64_t offset = pop_bytecode(translated, state);

  void*data = ar_alloc(length);
  memcpy(data, arena_get(&translated->constants,offset), length);
  ArgonObject *object = ARGON_NULL;
  switch (type) {
    case TYPE_OP_STRING:
      object = init_string_object(data, length);
      break;
  }
  state->registers[to_register] = object;
}

void run_instruction(Translated *translated, RuntimeState *state) {
  OperationType opcode = pop_bytecode(translated, state);
  switch (opcode) {
  case OP_LOAD_NULL:
    state->registers[pop_bytecode(translated, state)] = ARGON_NULL;
    break;
  case OP_LOAD_CONST:
    load_const(translated,state);
    break;
  }
}

void runtime(Translated translated) {
  RuntimeState state = {
      checked_malloc(translated.registerCount * sizeof(ArgonObject *)), 0};

  while (state.head < translated.bytecode.size)
    run_instruction(&translated, &state);
  free(state.registers);
}

static uint8_t siphash_key[16];

void generate_siphash_key() {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0 || read(fd, siphash_key, 16) != 16) {
    // Fallback or abort
  }
  close(fd);
}

uint64_t siphash64_bytes(const void *data, size_t len) {
  uint8_t out[8];
  if (siphash(data, len, siphash_key, out, sizeof(out)) != 0)
    return 0;

  uint64_t hash = 0;
  for (int i = 0; i < 8; ++i)
    hash |= ((uint64_t)out[i]) << (8 * i);

  return hash;
}