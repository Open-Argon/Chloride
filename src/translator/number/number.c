#include "../translator.h"
#include "number.h"
#include <gmp-x86_64.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_number(Translated *translated, ParsedValue *parsedValue) {
  char *number_str = (char*)parsedValue->data;
  size_t length = strlen(number_str);
  size_t number_pos = arena_push(&translated->constants, number_str, length);
  set_registers(translated, 1);
  
  size_t start = push_instruction_code(translated, OP_LOAD_CONST);
  push_instruction_code(translated, 0);

  push_instruction_code(translated, TYPE_OP_NUMBER);
  push_instruction_code(translated,length);
  push_instruction_code(translated, number_pos);
  return start;
}