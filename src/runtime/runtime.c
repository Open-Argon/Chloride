#include "runtime.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "internals/siphash/siphash.h"
#include "objects/type/type.h"
#include "objects/string/string.h"

void init_types() {
  init_type();
  init_string_type();
}

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