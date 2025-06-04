#ifndef CALL_H
#define CALL_H
#include "../../parser.h"
#include "../../../lexer/token.h"  // for Token

typedef struct {
  ParsedValue * to_call;
  DArray * args;
} ParsedCall;

// Function declaration for parsing an identifier
ParsedValue *parse_call(char *file, DArray *tokens, size_t *index,
                        ParsedValue *to_call);
void free_parse_call(void *ptr);

#endif // CALL_H