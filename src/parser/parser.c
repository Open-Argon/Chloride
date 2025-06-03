#include "parser.h"
#include "../dynamic_array/darray.h"
#include "../lexer/token.h"
#include "assign/assign.h"
#include "identifier/identifier.h"
#include "if/if.h"
#include "number/number.h"
#include "string/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ValueTypeNames[] = {"string", "assign", "identifier", "number", "if statement"};

void error_if_finished(char *file, DArray *tokens, size_t *index) {
  if ((*index) >= tokens->size) {
    Token *token = darray_get(tokens, tokens->size - 1);
    fprintf(stderr, "%s:%u:%u error: syntax error\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  }
}

ParsedValue *parse_token(char *file, DArray *parsed, DArray *tokens,
                         size_t *index, bool inline_flag) {
  Token *token = darray_get(tokens, *index);
  if (!inline_flag) {
    switch (token->type) {
    case TOKEN_IF:
      return parse_if(file, parsed, tokens, index);
    default:
      break;
    };
  }
  switch (token->type) {
  case TOKEN_STRING:
    (*index)++;
    return parse_string(*token);
  case TOKEN_NEW_LINE:
    while (token->type == TOKEN_NEW_LINE && ++(*index) < tokens->size) {
      token = darray_get(tokens, *index);
    }
    if (token->type == TOKEN_NEW_LINE)
      return NULL;
    return parse_token(file, parsed, tokens, index, inline_flag);
  case TOKEN_INDENT:
    fprintf(stderr, "%s:%u:%u error: invalid indentation\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  case TOKEN_IDENTIFIER:;
    ParsedValue *assign_to = parse_identifier(token);
    (*index)++;
    if (*index >= tokens->size)
      return assign_to;
    token = darray_get(tokens, *index);
    switch (token->type) {
    case TOKEN_ASSIGN:
    case TOKEN_ASSIGN_CARET:
    case TOKEN_ASSIGN_FLOORDIV:
    case TOKEN_ASSIGN_MINUS:
    case TOKEN_ASSIGN_MODULO:
    case TOKEN_ASSIGN_PLUS:
    case TOKEN_ASSIGN_SLASH:
    case TOKEN_ASSIGN_STAR:;
      DArray slice = darray_slice(parsed, parsed->size, parsed->size);
      return parse_assign(file, &slice, tokens, assign_to, index);
    default:
      return assign_to;
    }
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
    ParsedValue *assigning_to = darray_get(parsed, parsed->size - 1);
    fprintf(stderr, "%s:%u:%u error: cannot assign to %s\n", file, token->line,
            token->column, ValueTypeNames[assigning_to->type]);
    exit(EXIT_FAILURE);
  case TOKEN_NUMBER:
    (*index)++;
    return parse_number(token);
  default:
    fprintf(stderr, "Panic: unreachable\n");
    exit(EXIT_FAILURE);
  }
}

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag) {
  size_t index = 0;
  while (index < tokens->size) {
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
  case AST_IDENTIFIER:
  case AST_STRING:
    free(parsed->data);
    break;
  case AST_ASSIGN:
    free_parse_assign(parsed);
    break;
  case AST_NUMBER:
    break;
  case AST_IF:
    free_parsed_if(parsed);
    break;
  }
}