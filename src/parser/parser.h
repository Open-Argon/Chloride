#ifndef PARSER_H
#define PARSER_H

#include "../dynamic_array/darray.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct LinkedList LinkedList;

typedef enum {
  AST_STRING,
  AST_ASSIGN,
  AST_IDENTIFIER
} ValueType;

extern const char* ValueTypeNames[];

typedef struct {
  ValueType type;
  void *data;
} ParsedValue;

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag);

ParsedValue *parse_token(char *file, DArray *parsed, DArray *tokens,
                         size_t *index, bool inline_flag);

void free_parsed(void *ptr);

#endif // PARSER_H