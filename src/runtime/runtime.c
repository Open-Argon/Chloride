#include "runtime.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t pop_bytecode(Translated *translated, RuntimeState *state) {
  uint64_t *instruction = darray_get(&translated->bytecode, state->head++);
  return *instruction;
}

void run_instruction(Translated *translated, RuntimeState *state) {
  uint64_t opcode = pop_bytecode(translated, state);
  switch (opcode) { case OP_LOAD_NULL: pop_bytecode(translated, state);printf("null\n");}
}

void runtime(Translated translated) {
  RuntimeState state = {
      checked_malloc(translated.registerCount * sizeof(size_t)), 0};
    
  while (state.head < translated.bytecode.size)
    run_instruction(&translated, &state);
  free(state.registers);
}