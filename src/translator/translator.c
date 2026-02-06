/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "translator.h"
#include "../err.h"
#include "../hash_data/hash_data.h"
#include "../hashmap/hashmap.h"
#include "../parser/assignable/item/item.h"
#include "../parser/dictionary/dictionary.h"
#include "../parser/not/not.h"
#include "access/access.h"
#include "assignment/assignment.h"
#include "call/call.h"
#include "class/class.h"
#include "declaration/declaration.h"
#include "dowrap/dowrap.h"
#include "function/function.h"
#include "identifier/identifier.h"
#include "if/if.h"
#include "import/import.h"
#include "item_access/item_access.h"
#include "number/number.h"
#include "operation/operation.h"
#include "return/return.h"
#include "string/string.h"
#include "while/while.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void uint64_to_bytes(uint64_t value, uint8_t bytes[8]) {
  for (int i = 0; i < 8; i++) {
    bytes[i] = (value >> (i * 8)) & 0xFF;
  }
}

void arena_init(ConstantArena *arena) {
  arena->data = malloc(CHUNK_SIZE);
  arena->capacity = CHUNK_SIZE;
  arena->size = 0;
  arena->hashmap = createHashmap();
}

void arena_resize(ConstantArena *arena, size_t new_size) {
  size_t new_capacity = ((new_size / CHUNK_SIZE) + 1) * CHUNK_SIZE;
  if (new_capacity == arena->capacity)
    return;
  arena->data = realloc(arena->data, new_capacity);
  if (!arena->data) {
    fprintf(stderr, "error: failed to resize arena from %zu to %zu\n",
            new_capacity, arena->capacity);
    exit(EXIT_FAILURE);
  }
  arena->capacity = new_capacity;
}

size_t arena_push(ConstantArena *arena, const void *data, size_t length) {
  uint64_t hash = siphash64_bytes(data, length, siphash_key);

  // Look up offset in hashmap
  void *val = hashmap_lookup(arena->hashmap, hash);
  if (val != NULL) {
    size_t offset =
        (size_t)(uintptr_t)val - 1; // stored as pointer but really offset
    // Verify to avoid collision false positive
    if (memcmp(arena->data + offset, data, length) == 0) {
      return offset;
    }
  }

  // Not found: append data
  arena_resize(arena, arena->size + length);
  size_t offset = arena->size;
  memcpy(arena->data + arena->size, data, length);
  arena->size += length;

  // Insert into hashmap: store offset as pointer-sized integer
  hashmap_insert(arena->hashmap, hash, (void *)data,
                 (void *)(uintptr_t)offset + 1, 0);

  return offset;
}

Translated init_translator(char *path) {
  Translated translated;
  translated.path = path;
  translated.registerCount = 1;
  translated.registerAssignment = 1;
  translated.return_jumps = NULL;
  darray_init(&translated.bytecode, sizeof(uint8_t));
  arena_init(&translated.constants);
  return translated;
}

size_t push_instruction_byte(Translated *translator, uint8_t byte) {
  size_t offset = translator->bytecode.size;
  darray_push(&translator->bytecode, &byte);
  return offset;
}

void set_instruction_byte(Translated *translator, size_t index, uint8_t byte) {
  memcpy(translator->bytecode.data + index, &byte, sizeof(byte));
}

size_t push_instruction_code(Translated *translator, uint64_t code) {
  size_t offset = translator->bytecode.size;
  uint8_t bytes[8];
  uint64_to_bytes(code, bytes);
  for (size_t i = 0; i < sizeof(bytes); i++) {
    darray_push(&translator->bytecode, &(bytes[i]));
  }
  return offset;
}

void set_instruction_code(Translated *translator, size_t index, uint64_t code) {
  memcpy(translator->bytecode.data + index, &code, sizeof(code));
}

void set_registers(Translated *translator, uint8_t count) {
  if (count > translator->registerCount)
    translator->registerCount = count;
}

size_t translate_parsed(Translated *translated, ParsedValue *parsedValue,
                        ArErr *err) {
  switch (parsedValue->type) {
  case AST_STRING:
    return translate_parsed_string(translated,
                                   *((ParsedString *)parsedValue->data));
  case AST_DECLARATION:
    return translate_parsed_declaration(translated,
                                        *((DArray *)parsedValue->data), err);
  case AST_NUMBER:
    return translate_parsed_number(translated, (mpq_t *)parsedValue->data, 0);
  case AST_NULL:
    set_registers(translated, 1);
    size_t output = push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, 0);
    return output;
  case AST_BOOLEAN:
    set_registers(translated, 1);
    output = push_instruction_byte(translated, OP_LOAD_BOOL);
    push_instruction_byte(translated, (bool)parsedValue->data);
    return output;
  case AST_NEGATION:
    set_registers(translated, 1);
    output = translate_parsed(translated, parsedValue->data, err);
    push_instruction_byte(translated, OP_NEGATION);
    return output;
  case AST_FUNCTION:
    return translate_parsed_function(translated,
                                     (ParsedFunction *)parsedValue->data, err);
  case AST_IDENTIFIER:
    return translate_parsed_identifier(translated,
                                       (ParsedIdentifier *)parsedValue->data);
  case AST_IF:
    return translate_parsed_if(translated, (DArray *)parsedValue->data, err);
  case AST_WHILE:
    return translate_parsed_while(translated, (ParsedWhile *)parsedValue->data,
                                  err);
  case AST_DOWRAP:
    return translate_parsed_dowrap(translated, (DArray *)parsedValue->data,
                                   err);
  case AST_RETURN:
    return translate_parsed_return(translated,
                                   (ParsedReturn *)parsedValue->data, err);
  case AST_CALL:
    return translate_parsed_call(translated, (ParsedCall *)parsedValue->data,
                                 err);
  case AST_ACCESS:
    return translate_access(translated, (ParsedAccess *)parsedValue->data, err);
  case AST_ITEM_ACCESS:
    return translate_item_access(translated,
                                 (ParsedItemAccess *)parsedValue->data, err);
  case AST_OPERATION:
    return translate_operation(translated, (ParsedOperation *)parsedValue->data,
                               err);
  case AST_ASSIGN:
    return translate_parsed_assignment(translated,
                                       (ParsedAssign *)parsedValue->data, err);
  case AST_CLASS:
    return translate_parsed_class(translated, (ParsedClass *)parsedValue->data,
                                  err);
  case AST_TO_BOOL: {
    size_t first = translate_parsed(
        translated, ((ParsedToBool *)parsedValue->data)->value, err);
    push_instruction_byte(translated, OP_BOOL);
    if (((ParsedToBool *)parsedValue->data)->invert)
      push_instruction_byte(translated, OP_NOT);
    return first;
  }
  case AST_IMPORT:
    return translate_parsed_import(translated,
                                   (ParsedImport *)parsedValue->data, err);
  case AST_ARRAY: {
    DArray *arrayDarray = parsedValue->data;
    size_t first = push_instruction_byte(translated, OP_LOAD_CREATE_ARRAY);
    push_instruction_byte(translated, OP_INIT_CALL);
    push_instruction_code(translated, arrayDarray->size);
    for (size_t i = 0; i < arrayDarray->size; i++) {
      translate_parsed(translated, darray_get(arrayDarray, i), err);
      if (err->exists)
        return first;

      push_instruction_byte(translated, OP_INSERT_ARG);
      push_instruction_code(translated, i);
    }
    push_instruction_byte(translated, OP_CALL);
    return first;
  }
  case AST_DICTIONARY: {
    DArray *dictionaryDarray = parsedValue->data;

    size_t first = push_instruction_byte(translated, OP_CREATE_DICTIONARY);
    uint8_t dictionaryRegister = translated->registerAssignment++;

    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, 0);
    push_instruction_byte(translated, dictionaryRegister);

    push_instruction_byte(translated, OP_LOAD_SETITEM_METHOD);
    uint8_t setitemRegister = translated->registerAssignment++;
    set_registers(translated, translated->registerAssignment);

    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, 0);
    push_instruction_byte(translated, setitemRegister);

    for (size_t i = 0; i < dictionaryDarray->size; i++) {
      ParsedDictionaryEntry *entry = darray_get(dictionaryDarray, i);
      if (i != 0) {
        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, setitemRegister);
        push_instruction_byte(translated, 0);
      }
      push_instruction_byte(translated, OP_INIT_CALL);
      push_instruction_code(translated, 2);

      translate_parsed(translated, entry->key, err);
      if (err->exists)
        return first;

      push_instruction_byte(translated, OP_INSERT_ARG);
      push_instruction_code(translated, 0);

      translate_parsed(translated, entry->value, err);
      if (err->exists)
        return first;

      push_instruction_byte(translated, OP_INSERT_ARG);
      push_instruction_code(translated, 1);

      push_instruction_byte(translated, OP_CALL);
    }

    push_instruction_byte(translated, OP_COPY_TO_REGISTER);
    push_instruction_byte(translated, dictionaryRegister);
    push_instruction_byte(translated, 0);

    push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, dictionaryRegister);

    push_instruction_byte(translated, OP_LOAD_NULL);
    push_instruction_byte(translated, setitemRegister);

    translated->registerAssignment -= 2;
    return first;
  }
  }
  fprintf(stderr, "panic: undefined translation\n");
  exit(EXIT_FAILURE);
  return 0;
}

ArErr translate(Translated *translated, DArray *ast) {
  ArErr err = no_err;
  for (size_t i = 0; i < ast->size; i++) {
    ParsedValue *parsedValue = darray_get(ast, i);
    translate_parsed(translated, parsedValue, &err);
    if (err.exists) {
      break;
    }
  }
  return err;
}