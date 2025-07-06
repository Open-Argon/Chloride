#include "../translator.h"
#include "identifier.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_identifier(Translated *translated, char* identifier) {
  size_t length = strlen(identifier);
  size_t identifier_pos = arena_push(&translated->constants, identifier, length);
  set_registers(translated, 1);
  size_t start = push_instruction_byte(translated, OP_IDENTIFIER);
  push_instruction_code(translated,length);
  push_instruction_code(translated, identifier_pos);
  return start;
}