#include "../translator.h"
#include <stddef.h>
#include <string.h>

void translate_parsed_string(Translated *translated, ParsedValue *parsedValue) {
  size_t string_pos = arena_push_string(&translated->constants, (char*)parsedValue->data);
  // set_registers(translated, 1);
}