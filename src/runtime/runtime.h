#ifndef RUNTIME_H
#define RUNTIME_H
#include "../translator/translator.h"
#include "internals/hashmap/hashmap.h"

typedef struct {
  ArgonObject **registers;
  size_t head;
} RuntimeState;

void init_types();

void run_instruction(Translated *translated, RuntimeState *state);

void runtime(Translated translated);

uint64_t siphash64_bytes(const void *data, size_t len);

void generate_siphash_key();

#endif // RUNTIME_H