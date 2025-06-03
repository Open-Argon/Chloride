#include "number.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include "../../memory.h"
#include <gmp.h>

ParsedValue *parse_number(Token *token) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  mpz_t *number = checked_malloc(sizeof(mpz_t));
  mpz_init_set_str(*number, token->value, 10);
  parsedValue->type = AST_NUMBER;
  parsedValue->data = number;
  return parsedValue;
}