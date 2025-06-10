#ifndef IDENTIFIER_H
#define IDENTIFIER_H
#include "../../parser.h"
#include "../../../lexer/token.h"  // for Token

// Function declaration for parsing an identifier
ParsedValue * parse_identifier(Token * token);

#endif // IDENTIFIER_H