#ifndef RUNTIME_H
#define RUNTIME_H
#include "../translator/translator.h"

typedef struct {
  uint64_t *registers;
  size_t head;
} RuntimeState;

void init_types();

void run_instruction(Translated *translated, RuntimeState *state);

void runtime(Translated translated);

uint64_t siphash64_bytes(const void *data, size_t len);

void generate_siphash_key();

#endif // RUNTIME_H