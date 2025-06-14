#include "../translator.h"
#include "../../parser/string/string.h"
#include "string.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

size_t translate_parsed_string(Translated *translated, ParsedValue *parsedValue) {
  ParsedString *parsedString = (ParsedString*)parsedValue->data;
  size_t string_pos = arena_push(&translated->constants, parsedString->string, parsedString->length);
  set_registers(translated, 1);
  size_t start = push_instruction_code(translated, OP_LOAD_CONST);
  push_instruction_code(translated, 0);
  push_instruction_code(translated, TYPE_OP_STRING);
  push_instruction_code(translated,parsedString->length);
  push_instruction_code(translated, string_pos);
  return start;
}