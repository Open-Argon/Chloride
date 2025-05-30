#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "../../lexer/token.h"
#include "../parser.h"

// Declare functions related to string processing in parser

char *swap_quotes(char *input, char quote);

char *unquote(char *str);

ParsedValue *parse_string(Token token);

#endif // STRING_UTILS_H