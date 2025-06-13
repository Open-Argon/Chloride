#include "../translator.h"
#include <stddef.h>
#include <string.h>

void translate_parsed_string(Translated *translated, ParsedValue *parsedValue) {
  size_t string_pos = arena_push_string(&translated->constants, (char*)parsedValue->data);
  set_registers(translated, 1);
  push_instruction_code(translated, OP_LOAD_CONST);
  push_instruction_code(translated, 0);
  push_instruction_code(translated, OP_TYPE_STRING);
  push_instruction_code(translated,strlen(parsedValue->data)+1);
  push_instruction_code(translated, string_pos);
}