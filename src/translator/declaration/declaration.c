#include "declaration.h"
#include "../../parser/declaration/declaration.h"
#include "../translator.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
size_t translate_parsed_declaration(Translated *translated,
                                    DArray delcarations) {
  set_registers(translated, 1);
  size_t first = 0;
  for (size_t i = 0; i < delcarations.size; i++) {
    // TODO: add function delclaration
    ParsedSingleDeclaration *singleDeclaration = darray_get(&delcarations, i);
    size_t temp = translate_parsed(translated, singleDeclaration->from);
    if (i == 0)
      first = temp;
    size_t length = strlen(singleDeclaration->name);
    size_t offset =
        arena_push(&translated->constants, singleDeclaration->name, length);

    push_instruction_code(translated, OP_DECLARE);
    push_instruction_code(translated, length);
    push_instruction_code(translated, offset);
    push_instruction_code(translated, 0);
  }
  if (delcarations.size != 1) {
    push_instruction_code(translated, OP_LOAD_NULL);
    push_instruction_code(translated, 0);
  }
  return first;
}