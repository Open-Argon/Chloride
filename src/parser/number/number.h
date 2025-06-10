#ifndef NUMBER_H
#define NUMBER_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

// Function declaration for parsing an identifier
ParsedValue * parse_number(Token * token);

#endif // NUMBER_H