#include "list.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValueReturn parse_list(char *file, DArray *tokens, size_t *index) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_LIST;
  DArray *list = checked_malloc(sizeof(DArray));
  parsedValue->data = list;
  darray_init(list, sizeof(ParsedValue));
  (*index)++;
  skip_newlines_and_indents(tokens, index);
  error_if_finished(file, tokens, index);
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_RBRACKET) {
    while (true) {
      skip_newlines_and_indents(tokens, index);
      ParsedValueReturn parsedItem = parse_token(file, tokens, index, true);
      if (parsedItem.err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return parsedItem;
      }
      darray_push(list, parsedItem.value);
      free(parsedItem.value);
      error_if_finished(file, tokens, index);
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RBRACKET) {
        break;
      } else if (token->type != TOKEN_COMMA) {
        fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
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

void free_parsed_list(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed_list = parsedValue->data;
  darray_free(parsed_list, free_parsed);
  free(parsed_list);
}