#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "../../lexer/token.h"
#include "../parser.h"

// Declare functions related to string processing in parser

typedef struct {
  size_t length;
  char *string;
} ParsedString;

char *swap_quotes(char *input, char quote);

char *unquote(char *str, size_t *decoded_len);

ParsedValue *parse_string(char*file,Token* token);

#endif // STRING_UTILS_H