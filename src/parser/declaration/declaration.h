#ifndef DECLARATION_H
#define DECLARATION_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

typedef struct {
  char * name;
  ParsedValue * from;
  uint64_t line;
  uint64_t column;
} ParsedSingleDeclaration;

// Function declaration for parsing an identifier
ParsedValueReturn parse_declaration(char *file, DArray *tokens, size_t *index);

void free_declaration(void *ptr);

#endif // DECLARATION_H