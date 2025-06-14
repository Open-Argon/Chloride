#include "../translator.h"
#include "../../parser/string/string.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void translate_parsed_string(Translated *translated, ParsedValue *parsedValue) {
  ParsedString *parsedString = (ParsedString*)parsedValue->data;
  size_t string_pos = arena_push(&translated->constants, parsedString->string, parsedString->length);
  set_registers(translated, 1);
  push_instruction_code(translated, OP_LOAD_CONST);
  push_instruction_code(translated, 0);
  push_instruction_code(translated, OP_TYPE_STRING);
  push_instruction_code(translated,parsedString->length);
  push_instruction_code(translated, string_pos);
  fwrite(parsedString->string, 1, parsedString->length, stdout);
  putchar('\n');
}