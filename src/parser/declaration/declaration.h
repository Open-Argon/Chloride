#ifndef DECLARATION_H
#define DECLARATION_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

typedef struct {
  char * name;
  bool is_function;
  DArray * parameters; // string[]
  ParsedValue * from;
} ParsedSingleDeclaration;

// Function declaration for parsing an identifier
ParsedValue *parse_declaration(char *file, DArray *tokens,
                              size_t *index);

void free_declaration(void *ptr);

#endif // DECLARATION_H