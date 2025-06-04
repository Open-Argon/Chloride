#include "call.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *parse_call(char *file, DArray *tokens, size_t *index,
                        ParsedValue *to_call) {
  (*index)++;
  error_if_finished(file, tokens, index);
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedCall *parsedCall = checked_malloc(sizeof(ParsedCall));
  DArray *args = checked_malloc(sizeof(DArray));
  darray_init(args, sizeof(ParsedValue));
  parsedCall->to_call = to_call;
  parsedCall->args = args;
  parsedValue->type = AST_ACCESS;
  parsedValue->data = parsedCall;
  Token *token = darray_get(tokens, *index);
  DArray *arg = checked_malloc(sizeof(DArray));
  darray_init(arg, sizeof(ParsedValue));
  while (true) {
    bool to_break = false;
    switch (token->type) {
    case TOKEN_RPAREN:
      to_break = true;
      break;
    default:
      break;
    }
    if (to_break)
      break;
    ParsedValue *parsedValue = parse_token(file, tokens, index, true);
    darray_push(arg, parsedValue);
    switch (token->type) {
    case TOKEN_COMMA:
      darray_push(args, arg);
      arg = checked_malloc(sizeof(DArray));
      darray_init(arg, sizeof(ParsedValue));
      break;
    default:
      break;
    }
  }
  return parsedValue;
}

void free_parse_call(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedCall *parsedCall = parsedValue->data;

  free_parsed(parsedCall->to_call);

  

  free(parsedCall);
}