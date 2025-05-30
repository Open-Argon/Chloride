#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>


typedef struct LinkedList LinkedList;

typedef enum {
  AST_STRING,
} ValueType;

typedef struct {
  ValueType type;
  void *data;
  
} TaggedValue;

void parser(LinkedList *parsed, LinkedList *tokens, bool inline_flag);

TaggedValue *parse_token(LinkedList *tokens, size_t *index);

void free_tagged_value(void *ptr);


#endif // PARSER_H