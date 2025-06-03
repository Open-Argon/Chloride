#include "if.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../memory.h"

ParsedValue *parse_if(char *file, DArray *parsed, DArray *tokens,
                      size_t *index) {
  (*index)++;
  error_if_finished(file, tokens, index);
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_LPAREN) {
    fprintf(stderr,
            "%s:%u:%u error: if statement requires paren for the condition\n",
            file, token->line, token->column);
    exit(EXIT_FAILURE);
  }
  (*index)++;
  error_if_finished(file, tokens, index);
  DArray *parsed_if = checked_malloc(sizeof(DArray));
  darray_init(parsed_if, sizeof(ParsedConditional));
  DArray *condition = checked_malloc(sizeof(DArray));
  darray_init(condition, sizeof(ParsedValue));
  while ((*index) < tokens->size) {
    ParsedValue *parsed_code = parse_token(file, parsed, tokens, index, true);
    if (parsed_code) {
      darray_push(condition, parsed_code);
      free(parsed_code);
    }
    token = darray_get(tokens, *index);
    if (token->type == TOKEN_RPAREN)
      break;
  }
  if (token->type != TOKEN_RPAREN) {
    fprintf(stderr,
            "%s:%u:%u error: missing closing parenthesis in if condition\n",
            file, token->line, token->column);
    exit(EXIT_FAILURE);
  }
  (*index)++;
  error_if_finished(file, tokens, index);
  ParsedValue *parsed_content = parse_token(file, parsed, tokens, index, true);
  if (!parsed_content) {
    fprintf(stderr, "%s:%u:%u error: expected body after if condition\n", file,
            token->line, token->column);
    exit(EXIT_FAILURE);
  }
  ParsedConditional output_conditional = {condition, parsed_content};
  darray_push(parsed_if, &output_conditional);
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_IF;
  parsedValue->data = parsed_if;
  return parsedValue;
}

void free_conditional(void *ptr) {
  ParsedConditional *conditional = ptr;
  darray_free(conditional->condition, free_parsed);
  free_parsed(conditional->content);
}

void free_parsed_if(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed_if = parsedValue->data;
  darray_free(parsed_if, free_conditional);
}