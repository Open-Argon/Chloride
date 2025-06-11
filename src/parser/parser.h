#ifndef PARSER_H
#define PARSER_H

#include "../dynamic_array/darray.h"
#include <stdbool.h>
#include <stddef.h>

#define SWITCH_OPERATIONS case TOKEN_AND:\
case TOKEN_OR:\
case TOKEN_NOT_IN:\
case TOKEN_IN:\
case TOKEN_LE:\
case TOKEN_GE:\
case TOKEN_LT:\
case TOKEN_GT:\
case TOKEN_NE:\
case TOKEN_EQ:\
case TOKEN_PLUS:\
case TOKEN_MINUS:\
case TOKEN_MODULO:\
case TOKEN_STAR:\
case TOKEN_FLOORDIV:\
case TOKEN_SLASH:\
case TOKEN_CARET:

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
  AST_DOWRAP,
  AST_OPERATION,
  AST_LIST
} ValueType;

extern const char* ValueTypeNames[];

typedef struct {
  ValueType type;
  void *data;
} ParsedValue;

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag);

ParsedValue *parse_token_full(char *file, DArray *tokens, size_t *index,
                         bool inline_flag, bool process_operations);

ParsedValue *parse_token(char *file, DArray *tokens, size_t *index,
                         bool inline_flag);

void free_parsed(void *ptr);

void error_if_finished(char *file,DArray *tokens, size_t *index);

size_t skip_newlines_and_indents(DArray *tokens, size_t *index);

#endif // PARSER_H