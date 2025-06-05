#ifndef PARSER_H
#define PARSER_H

#include "../dynamic_array/darray.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct LinkedList LinkedList;

typedef enum {
  AST_STRING,
  AST_ASSIGN,
  AST_IDENTIFIER,
  AST_NUMBER,
  AST_IF,
  AST_ACCESS,
  AST_CALL,
  AST_DECLARATION,
  AST_NULL,
  AST_BOOLEAN,
  AST_DOWRAP
} ValueType;

extern const char* ValueTypeNames[];

typedef struct {
  ValueType type;
  void *data;
} ParsedValue;

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag);

ParsedValue *parse_token(char *file, DArray *tokens,
                         size_t *index, bool inline_flag);

void free_parsed(void *ptr);

void error_if_finished(char *file,DArray *tokens, size_t *index);

void skip_newlines_and_indents(DArray *tokens, size_t *index);

#endif // PARSER_H