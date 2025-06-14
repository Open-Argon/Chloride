#include "access.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../string/string.h"
#include "../../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *parse_access(char *file, DArray *tokens, size_t *index,
                          ParsedValue *to_access) {
  Token *first_token = darray_get(tokens, *index);
  (*index)++;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedAccess *parsedAccess = checked_malloc(sizeof(ParsedAccess));
  parsedAccess->to_access = *to_access;
  free(to_access);
  darray_init(&parsedAccess->access, sizeof(ParsedValue));
  if (first_token->type == TOKEN_DOT) {
    error_if_finished(file, tokens, index);
    Token *token = darray_get(tokens, *index);
    ParsedValue *parsedString = parse_string(token, false);
    darray_push(&parsedAccess->access, parsedString);
    free(parsedString);
  } else {
    while (true) {
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      ParsedValue *parsedValue = parse_token(file, tokens, index, true);
      darray_push(&parsedAccess->access, parsedValue);
      free(parsedValue);
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      Token *token = darray_get(tokens, *index);
      if (token->type == TOKEN_RBRACKET) {
        break;
      } else if (token->type != TOKEN_COLON) {
        fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file,
                token->line, token->column);
        exit(EXIT_FAILURE);
      }
      (*index)++;
    }
  }
  parsedValue->type = AST_ACCESS;
  parsedValue->data = parsedAccess;
  (*index)++;
  return parsedValue;
}

void free_parse_access(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAccess *parsedAccess = parsedValue->data;
  free_parsed(&parsedAccess->to_access);
  darray_free(&parsedAccess->access, free_parsed);
  free(parsedAccess);
}