// parser.h

#ifndef iF_H
#define iF_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

typedef struct {
  DArray *condition; // NULL for 'else'
  ParsedValue *content;
} ParsedConditional;

ParsedValue *parse_if(char *file, DArray *parsed, DArray *tokens,
                      size_t *index);

void free_parsed_if(void *ptr);

#endif // iF_H