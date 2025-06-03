#include "number.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include <stdlib.h>
#include <gmp.h>

ParsedValue *parse_number(Token *token) {
  ParsedValue *parsedValue = malloc(sizeof(ParsedValue));
  mpz_t *number = malloc(sizeof(mpz_t));
  mpz_init_set_str(*number, token->value, 10);
  parsedValue->type = AST_NUMBER;
  parsedValue->data = number;
  return parsedValue;
}