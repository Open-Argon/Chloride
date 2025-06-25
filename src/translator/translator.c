#include "translator.h"
#include "declaration/declaration.h"
#include "function/function.h"
#include "number/number.h"
#include "string/string.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void arena_init(ConstantArena *arena) {
  arena->data = checked_malloc(CHUNK_SIZE);
  arena->capacity = CHUNK_SIZE;
  arena->size = 0;
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

void arena_free(ConstantArena *arena) {
  free(arena->data);
  arena->capacity = 0;
  arena->size = 0;
}

void *arena_get(ConstantArena *arena, size_t offset) {
  return arena->data + offset;
}

size_t arena_push(ConstantArena *arena, const void *data, size_t length) {
  if (arena->size >= length) {
    for (size_t i = 0; i <= (arena->size - length); i++) {
      if (memcmp(data, arena->data + i, length) == 0) {
        return i;
      }
    }
  }
  arena_resize(arena, arena->size + length);
  size_t offset = arena->size;
  memcpy(arena->data + arena->size, data, length);
  arena->size += length;
  return offset;
}

Translated init_translator() {
  Translated translated;
  translated.registerCount = 0;
  darray_init(&translated.bytecode, sizeof(uint64_t));
  arena_init(&translated.constants);
  return translated;
}

void set_instruction_code(Translated *translator, size_t offset,
                          uint64_t code) {
  size_t *ptr = (translator->bytecode.data + offset);
  *ptr = code;
}

size_t push_instruction_code(Translated *translator, uint64_t code) {
  code = htole64(code);
  size_t offset = translator->bytecode.size;
  darray_push(&translator->bytecode, &code);
  return offset;
}

void set_registers(Translated *translator, size_t count) {
  if (count > translator->registerCount)
    translator->registerCount = count;
}

size_t translate_parsed(Translated *translated, ParsedValue *parsedValue) {
  switch (parsedValue->type) {
  case AST_STRING:
    return translate_parsed_string(translated,
                                   *((ParsedString *)parsedValue->data));
  case AST_DECLARATION:
    return translate_parsed_declaration(translated,
                                        *((DArray *)parsedValue->data));
  case AST_NUMBER:
    return translate_parsed_number(translated, (char *)parsedValue->data, 0);
  case AST_NULL:
    set_registers(translated, 1);
    size_t output = push_instruction_code(translated, OP_LOAD_NULL);
    push_instruction_code(translated, 0);
    return output;
  case AST_FUNCTION:
    return translate_parsed_function(translated,
                                     (ParsedFunction *)parsedValue->data);
  }
  return 0;
}

void translate(Translated *translated, DArray *ast) {
  for (size_t i = 0; i < ast->size; i++) {
    ParsedValue *parsedValue = darray_get(ast, i);
    translate_parsed(translated, parsedValue);
  }
}

void free_translator(Translated *translated) {
  darray_free(&translated->bytecode, NULL);
  arena_free(&translated->constants);
}