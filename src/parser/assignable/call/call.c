#include "call.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_call(char *file, DArray *tokens, size_t *index,
                             ParsedValue *to_call) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedCall *call = checked_malloc(sizeof(ParsedCall));
  call->to_call = to_call;
  parsedValue->data = call;
  parsedValue->type = AST_CALL;
  darray_init(&call->args, sizeof(ParsedValue));
  (*index)++;
  error_if_finished(file, tokens, index);
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_RPAREN) {
    while ((*index) < tokens->size) {
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      ParsedValueReturn parsedArg = parse_token(file, tokens, index, true);
      if (parsedArg.err.exists) {
        free_parsed(to_call);
        free(to_call);
        free_parsed(parsedValue);
        free(parsedValue);
        return parsedArg;
      } else if (!parsedArg.value) {
        free_parsed(to_call);
        free(to_call);
        free_parsed(parsedValue);
        free(parsedValue);

        return (ParsedValueReturn){
            create_err(token->line, token->column, token->length, file,
                       "Syntax Error", "expected argument"),
            NULL};
      }
      darray_push(&call->args, parsedArg.value);
      free(parsedArg.value);
      error_if_finished(file, tokens, index);
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RPAREN) {
        break;
      } else if (token->type != TOKEN_COMMA) {
        fprintf(stderr, "%s:%zu:%zu error: expected comma\n", file, token->line,
                token->column);
        exit(EXIT_FAILURE);
      }
      (*index)++;
      error_if_finished(file, tokens, index);
    }
  }
  (*index)++;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_call(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedCall *parsedCall = parsedValue->data;

  darray_free(&parsedCall->args, free_parsed);
  free_parsed(parsedCall->to_call);
  free(parsedCall->to_call);
  free(parsedCall);
}