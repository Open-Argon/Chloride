/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "../dynamic_array/darray.h"
#include "../hashmap/hashmap.h"
#include "../memory.h"
#include "../parser/parser.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
  OP_LOAD_STRING,
  OP_DECLARE,
  OP_LOAD_NULL,
  OP_LOAD_FUNCTION,
  OP_IDENTIFIER,
  OP_BOOL,
  OP_JUMP_IF_FALSE,
  OP_JUMP,
  OP_NEW_SCOPE,
  OP_EMPTY_SCOPE,
  OP_POP_SCOPE,
  OP_INIT_CALL,
  OP_INSERT_ARG,
  OP_CALL,
  OP_SOURCE_LOCATION,
  OP_LOAD_BOOL,
  OP_LOAD_NUMBER,
  OP_ASSIGN,
  OP_COPY_TO_REGISTER,
  OP_ADDITION,
  OP_SUBTRACTION,
  OP_LOAD_ACCESS_FUNCTION,
  OP_MULTIPLICATION,
  OP_DIVISION,
  OP_NOT
} OperationType;

void arena_resize(ConstantArena *arena, size_t new_size);

size_t arena_push(ConstantArena *arena, const void *data, size_t length);

size_t push_instruction_byte(Translated *translator, uint8_t byte);

void set_instruction_byte(Translated *translator, size_t index, uint8_t byte);

size_t push_instruction_code(Translated *translator, uint64_t code);

void set_instruction_code(Translated *translator, size_t index, uint64_t code);

void set_registers(Translated *translator, uint8_t count);

Translated init_translator(char *path);

size_t translate_parsed(Translated *translated, ParsedValue *parsedValue,
                        ArErr *err);

ArErr translate(Translated *translated, DArray *ast);

#endif