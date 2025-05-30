#include "parser.h"
#include "../dynamic_array/darray.h"
#include "../lexer/token.h"
#include "string/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValue *parse_token(DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  switch (token->type) {
  case TOKEN_STRING:
    (*index)++;
    return parse_string(*token);
  case TOKEN_NEW_LINE:
    (*index)++;
    return NULL;
  case TOKEN_INDENT:
    fprintf(stderr, "error: \n");
    exit(EXIT_FAILURE);
  default:
    fprintf(stderr, "Panic: unreachable\n");
    exit(EXIT_FAILURE);
  }
}

void parser(DArray *parsed, DArray *tokens, bool inline_flag) {
  size_t index = 0;
  size_t length = tokens->size;
  while (index < length) {
    ParsedValue *parsed_code = parse_token(tokens, &index);
    if (parsed_code) {
      darray_push(parsed, parsed_code);
      free(parsed_code);
    }
  }
}

void free_parsed_value(void *ptr) {
  ParsedValue *tagged = ptr;
  switch (tagged->type) {
  case AST_STRING:
    free(tagged->data);
    break;
  }
}