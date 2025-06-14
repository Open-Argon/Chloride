#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "../dynamic_array/darray.h"
#include <stddef.h>
#include "../dynamic_array/darray.h"
#include "../parser/parser.h"
#include "../memory.h"

typedef enum { OP_LOAD_CONST=255 } OperationType;
typedef enum { OP_TYPE_STRING=255 } types;

typedef struct {
  void *data;
  size_t capacity;
  size_t size;
} ConstantArena;

typedef struct {
  size_t registerCount;
  DArray bytecode;
  ConstantArena constants;
} Translated;

void * arena_get(ConstantArena *arena, size_t offset);

size_t arena_push(ConstantArena *arena, const void *data, size_t length);

size_t push_instruction_code(Translated * translator, size_t code);

void set_registers(Translated * translator, size_t count);

Translated init_translator();

void translate(Translated * translator, DArray *ast);

void free_translator(Translated * translated);

#endif