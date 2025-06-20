#ifndef RUNTIME_H
#define RUNTIME_H
#include "../translator/translator.h"

typedef struct {
  uint64_t *registers;
  size_t head;
} RuntimeState;


typedef struct {

} ArObject;

void run_instruction(Translated *translated, RuntimeState *state);

void runtime(Translated translated);

#endif // RUNTIME_H