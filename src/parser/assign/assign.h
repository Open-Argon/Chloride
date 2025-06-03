#ifndef ASSIGN_H
#define ASSIGN_H
#include "../parser.h"
#include "../../lexer/token.h"

typedef struct {
  bool let;
  ParsedValue * to;
  TokenType type;
  ParsedValue * from;
} ParsedAssign;

ParsedValue *parse_assign(char*file,DArray *parsed, DArray *tokens,
                          ParsedValue *assign_to, size_t *index);

void free_parse_assign(void*ptr);

#endif // ASSIGN_H