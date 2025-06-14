#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "../dynamic_array/darray.h"
#include "../memory.h"
#include "../parser/parser.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { OP_LOAD_CONST = 255, OP_DECLARE, OP_LOAD_NULL, OP_JUMP } OperationType;
typedef enum { TYPE_OP_STRING = 255, TYPE_OP_NUMBER } types;

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

void *arena_get(ConstantArena *arena, size_t offset);

size_t arena_push(ConstantArena *arena, const void *data, size_t length);

void set_instruction_code(Translated * translator, size_t offset, uint64_t code);

size_t push_instruction_code(Translated *translator, uint64_t code);

void set_registers(Translated *translator, size_t count);

Translated init_translator();

size_t translate_parsed(Translated * translator, ParsedValue * parsedValue);

void translate(Translated *translator, DArray *ast);

void free_translator(Translated *translated);

#endif