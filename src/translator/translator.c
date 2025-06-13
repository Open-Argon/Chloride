#include "translator.h"
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
  new_size = ((new_size / CHUNK_SIZE) + 1)*CHUNK_SIZE;
  if (new_size == arena->capacity)
    return;
  arena->data = realloc(arena->data, new_size);
  if (!arena->data) {
    fprintf(stderr, "error: failed to resize arena from %zu to %zu\n", new_size, arena->capacity);
    exit(EXIT_FAILURE);
  }
  arena->capacity = new_size;
}


void arena_free(ConstantArena *arena) {
  free(arena->data);
  arena->capacity = 0;
  arena->size = 0;
}

void * arena_get(ConstantArena *arena, size_t offset) {
  return arena->data + offset;
}


size_t arena_push_string(ConstantArena *arena, const char *string) {
  size_t length = strlen(string)+1;
  arena_resize(arena, arena->size+length);
  size_t offset = arena->size;
  strcpy(arena->data + arena->size, string);
  arena->size += length;
  return offset;
}

size_t arena_push(ConstantArena *arena, const void *data, size_t length) {
  arena_resize(arena, arena->size+length);
  size_t offset = arena->size;
  memcpy(arena->data + arena->size, data, length);
  arena->size += length;
  return offset;
}

Translated init_translator() {
  Translated translated;

  darray_init(&translated.bytecode, sizeof(uint8_t));
  arena_init(&translated.constants);
  return translated;
}

size_t push_instruction_code(Translated * translator, size_t code) {
  size_t offset = translator->bytecode.size;
  darray_push(&translator->bytecode, &code);
  return offset;
}

void set_registers(Translated * translator, size_t count) {
  if (count>translator->registerCount) translator->registerCount = count;
}

void translate_parsed(Translated * translator, ParsedValue * parsedValue) {
  switch (parsedValue->type) {
    case AST_STRING:
      translate_parsed_string(translator,parsedValue);
  }
}

void translate(Translated * translator, DArray *ast) {
  for (size_t i = 0; i<ast->size; i++) {
    ParsedValue * parsedValue = darray_get(ast, i);
    translate_parsed(translator,parsedValue);
  }
}

void free_translator(Translated * translated) {
  darray_free(&translated->bytecode, NULL);
  arena_free(&translated->constants);
}