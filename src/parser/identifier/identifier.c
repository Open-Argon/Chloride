#include "identifier.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include <stdlib.h>
#include <string.h>

ParsedValue *parse_identifier(Token *token) {
  ParsedValue *parsedValue = malloc(sizeof(ParsedValue));
  parsedValue->type = AST_IDENTIFIER;
  parsedValue->data = strcpy(malloc(sizeof(token->value)), token->value);
  return parsedValue;
}