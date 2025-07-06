#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "../dynamic_array/darray.h"
#include "../memory.h"
#include "../parser/parser.h"
#include "../hashmap/hashmap.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { OP_LOAD_CONST, OP_DECLARE, OP_LOAD_NULL, OP_LOAD_FUNCTION, OP_IDENTIFIER } OperationType;
typedef enum { TYPE_OP_STRING, TYPE_OP_NUMBER } types;

typedef struct {
  void *data;
  size_t capacity;
  size_t size;
  struct hashmap * hashmap;
} ConstantArena;

typedef struct {
  uint8_t registerCount;
  DArray bytecode;
  ConstantArena constants;
} Translated;

void arena_resize(ConstantArena *arena, size_t new_size);

void *arena_get(ConstantArena *arena, size_t offset);

size_t arena_push(ConstantArena *arena, const void *data, size_t length);

size_t push_instruction_byte(Translated *translator, uint8_t byte);

size_t push_instruction_code(Translated *translator, uint64_t code);

void set_registers(Translated *translator, uint8_t count);

Translated init_translator();

size_t translate_parsed(Translated * translator, ParsedValue * parsedValue);

void translate(Translated *translator, DArray *ast);

void free_translator(Translated *translated);

#endif