#include "../translator.h"
#include "declaration.h"
#include "../../parser/declaration/declaration.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
size_t translate_parsed_declaration(Translated *translated,
                                  ParsedValue *parsedValue) {
  DArray *delcarations = (DArray *)parsedValue->data;
  set_registers(translated, 2);
  size_t first = 0;
  for (size_t i = 0; i < delcarations->size; i++) {
    // TODO: add function delclaration
    ParsedSingleDeclaration*singleDeclaration = darray_get(delcarations, i);
    size_t temp = translate_parsed(translated, singleDeclaration->from);
    if (i==0) first = temp;
    size_t length = strlen(singleDeclaration->name);
    size_t offset = arena_push(&translated->constants, singleDeclaration->name, length);
    
    push_instruction_code(translated, OP_LOAD_CONST);
    push_instruction_code(translated, 1);
    push_instruction_code(translated, TYPE_OP_STRING);
    push_instruction_code(translated,length);
    push_instruction_code(translated, offset);

    push_instruction_code(translated, OP_DECLARE);
    push_instruction_code(translated, 0);
    push_instruction_code(translated, 1);
  }
  if (delcarations->size != 1) {
    push_instruction_code(translated, OP_LOAD_NULL);
    push_instruction_code(translated, 0);
  }
  return first;
}