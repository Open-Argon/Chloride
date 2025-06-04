#ifndef DECLARATION_H
#define DECLARATION_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

typedef struct {
  char * name;
  bool is_function;
  DArray args; // string[]
  ParsedValue * from;
} ParsedDeclaration;

// Function declaration for parsing an identifier
ParsedValue *parse_declaration(char *file, DArray *parsed, DArray *tokens, size_t *index);

#endif // DECLARATION_H