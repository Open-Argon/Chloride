#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include "../dynamic_array/darray.h"


typedef struct LinkedList LinkedList;

typedef enum {
  AST_STRING,
  AST_ASSIGN,
} ValueType;

typedef struct {
  ValueType type;
  void *data;
  
} ParsedValue;

void parser(char*file,DArray *parsed, DArray *tokens, bool inline_flag);

ParsedValue *parse_token(char*file,DArray *parsed, DArray *tokens, size_t *index, bool inline_flag);

void free_parsed(void *ptr);


#endif // PARSER_H