#include "../translator.h"
#include "number.h"
#include <gmp.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_number(Translated *translated, char *number_str, size_t to_register) {
  size_t length = strlen(number_str);
  size_t number_pos = arena_push(&translated->constants, number_str, length);
  set_registers(translated, to_register+1);
  
  size_t start = push_instruction_code(translated, OP_LOAD_CONST);
  push_instruction_code(translated, to_register);

  push_instruction_code(translated, TYPE_OP_NUMBER);
  push_instruction_code(translated,length);
  push_instruction_code(translated, number_pos);
  return start;
}