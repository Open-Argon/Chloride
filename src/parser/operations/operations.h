#ifndef OPERATIONS_H
#define OPERATIONS_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

typedef struct {
  TokenType operation;
  DArray to_operate_on; // ParsedValue[]
} ParsedOperation;

ParsedValue *parse_operations(char*file,DArray *tokens, size_t * index, ParsedValue * first_parsed_value);

void free_operation(void *ptr);

#endif // OPERATIONS_H