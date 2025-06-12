#ifndef ACCESS_H
#define ACCESS_H
#include "../../parser.h"
#include "../../../lexer/token.h"  // for Token

typedef struct {
  ParsedValue to_access;
  DArray access;
} ParsedAccess;

// Function declaration for parsing an identifier
ParsedValue *parse_access(char*file,DArray *tokens, size_t * index, ParsedValue * to_access);

void free_parse_access(void *ptr);

#endif // ACCESS_H