/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "../dynamic_array/darray.h"
#include "../memory.h"
#include "../parser/parser.h"
#include "../hashmap/hashmap.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { OP_LOAD_CONST, OP_DECLARE, OP_LOAD_NULL, OP_LOAD_FUNCTION, OP_IDENTIFIER, OP_BOOL, OP_JUMP_IF_FALSE, OP_JUMP, OP_NEW_SCOPE, OP_POP_SCOPE } OperationType;
typedef enum { TYPE_OP_STRING, TYPE_OP_NUMBER } types;

typedef struct {
  void *data;
  size_t capacity;
  size_t size;
  struct hashmap * hashmap;
} ConstantArena;

typedef struct {
  uint8_t registerCount;
  DArray *return_jumps;
  DArray bytecode;
  DArray source_locations;
  ConstantArena constants;
} Translated;

typedef struct {
  uint64_t line;
  uint64_t column;
  uint64_t length;
} SourceLocation;

void arena_resize(ConstantArena *arena, size_t new_size);

void *arena_get(ConstantArena *arena, size_t offset);

size_t arena_push(ConstantArena *arena, const void *data, size_t length);

size_t push_instruction_byte(Translated *translator, uint8_t byte);

void set_instruction_byte(Translated *translator, size_t index, uint8_t byte);

size_t push_instruction_code(Translated *translator, uint64_t code);

void set_instruction_code(Translated *translator, size_t index, uint64_t code);

void set_registers(Translated *translator, uint8_t count);

Translated init_translator();

size_t translate_parsed(Translated *translated, ParsedValue *parsedValue);

void translate(Translated *translated, DArray *ast);

void free_translator(Translated *translated);

#endif