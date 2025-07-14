#include "return.h"

size_t translate_parsed_return(Translated *translated,
                               ParsedReturn *parsedReturn) {

  size_t first = translate_parsed(translated, parsedReturn->value);
  push_instruction_byte(translated, OP_JUMP);
  size_t return_up = push_instruction_code(translated, 0);
  darray_push(translated->return_jumps, &return_up);
  return first;
}