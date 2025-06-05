#include "identifier.h"
#include "../../../lexer/token.h"
#include "../../parser.h"
#include <string.h>
#include "../../../memory.h"

ParsedValue *parse_identifier(Token *token) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_IDENTIFIER;
  parsedValue->data = strcpy(checked_malloc(strlen(token->value) + 1), token->value);
  return parsedValue;
}