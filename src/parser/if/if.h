#ifndef iF_H
#define iF_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

typedef struct {
  ParsedValue * condition; // NULL for 'else'
  ParsedValue *content;
} ParsedConditional;

ParsedValue *parse_if(char *file, DArray *tokens,
                      size_t *index);

void free_parsed_if(void *ptr);

#endif // iF_H