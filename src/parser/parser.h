#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include "../dynamic_array/darray.h"


typedef struct LinkedList LinkedList;

typedef enum {
  AST_STRING,
} ValueType;

typedef struct {
  ValueType type;
  void *data;
  
} ParsedValue;

void parser(DArray * parsed, DArray * tokens, bool inline_flag);

ParsedValue * parse_token(DArray * tokens, size_t *index);

void free_parsed_value(void *ptr);


#endif // PARSER_H