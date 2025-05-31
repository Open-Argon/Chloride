#include "parser.h"
#include "../dynamic_array/darray.h"
#include "../lexer/token.h"
#include "assign/assign.h"
#include "string/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *parse_token(char *file, DArray *parsed, DArray *tokens,
                         size_t *index, bool inline_flag) {
  Token *token = darray_get(tokens, *index);
  if (!inline_flag) {
    switch (token->type) {
      default:
        break;
    };
  }
  switch (token->type) {
  case TOKEN_STRING:
    (*index)++;
    return parse_string(*token);
  case TOKEN_NEW_LINE:
    while (token->type  == TOKEN_NEW_LINE && *index+1 <= tokens->size) {
      (*index)++;
      token = darray_get(tokens, *index);
    }
    if (token->type == TOKEN_NEW_LINE) return NULL;
    return parse_token(file, parsed, tokens, index, inline_flag);
  case TOKEN_INDENT:
    fprintf(stderr, "%s:%u:%u error: invalid indentation\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  case TOKEN_ASSIGN:
  case TOKEN_ASSIGN_CARET:
  case TOKEN_ASSIGN_FLOORDIV:
  case TOKEN_ASSIGN_MINUS:
  case TOKEN_ASSIGN_MODULO:
  case TOKEN_ASSIGN_PLUS:
  case TOKEN_ASSIGN_SLASH:
  case TOKEN_ASSIGN_STAR:
    if (parsed->size == 0) {
      fprintf(stderr, "%s:%u:%u error: syntax error\n", file, token->line,
            token->column);
      exit(EXIT_FAILURE);
    }
    ParsedValue *assign_to = malloc(sizeof(ParsedValue));
    memcpy(assign_to, darray_get(parsed, parsed->size-1), sizeof(ParsedValue));
    darray_resize(parsed, parsed->size-1);
    return parse_assign(file, parsed, tokens, assign_to, index);
  default:
    fprintf(stderr, "Panic: unreachable\n");
    exit(EXIT_FAILURE);
  }
}

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag) {
  size_t index = 0;
  size_t length = tokens->size;
  while (index < length) {
    ParsedValue *parsed_code =
        parse_token(file, parsed, tokens, &index, inline_flag);
    if (parsed_code) {
      darray_push(parsed, parsed_code);
      free(parsed_code);
    }
  }
}

void free_parsed(void *ptr) {
  ParsedValue *parsed = ptr;
  switch (parsed->type) {
  case AST_STRING:
    free(parsed->data);
    break;
  case AST_ASSIGN:
    free_parse_assign(parsed);
    break;
  }
}