#include "parser.h"
#include "../dynamic_array/darray.h"
#include "../lexer/token.h"
#include "assignable/assign/assign.h"
#include "assignable/identifier/identifier.h"
#include "declaration/declaration.h"
#include "if/if.h"
#include "number/number.h"
#include "string/string.h"
#include "literals/literals.h"
#include "assignable/call/call.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ValueTypeNames[] = {"string", "assign",       "identifier",
                                "number", "if statement", "access",
                                "call", "declaration", "null", "boolean"};

void error_if_finished(char *file, DArray *tokens, size_t *index) {
  if ((*index) >= tokens->size) {
    Token *token = darray_get(tokens, tokens->size - 1);
    fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  }
}

ParsedValue *parse_token(char *file, DArray *tokens, size_t *index,
                         bool inline_flag) {
  Token *token = darray_get(tokens, *index);

  ParsedValue *output = NULL;

  if (!inline_flag) {
    switch (token->type) {
    case TOKEN_IF:
      return parse_if(file, tokens, index);
    default:
      break;
    };
  }
  switch (token->type) {
  case TOKEN_TRUE:
    (*index)++;
    output = parse_true();
    break;
  case TOKEN_FALSE:
    (*index)++;
    output = parse_false();
    break;
  case TOKEN_NULL:
    (*index)++;
    output = parse_null();
    break;
  case TOKEN_STRING:
    (*index)++;
    output = parse_string(*token);
    break;
  case TOKEN_NEW_LINE:
    while (token->type == TOKEN_NEW_LINE && ++(*index) < tokens->size) {
      token = darray_get(tokens, *index);
    }
    if (token->type == TOKEN_NEW_LINE)
      break;
    output = parse_token(file, tokens, index, inline_flag);
    break;
  case TOKEN_INDENT:
    fprintf(stderr, "%s:%zu:%zu error: invalid indentation\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  case TOKEN_IDENTIFIER:
    (*index)++;
    output = parse_identifier(token);
    break;
  case TOKEN_NUMBER:
    (*index)++;
    output = parse_number(token);
    break;
  case TOKEN_LET:
    output = parse_declaration(file, tokens, index);
    break;
  default:
    fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  }

  // LHS required
  bool passed = false;
  while (!passed && (*index) < tokens->size) {
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
      output = parse_assign(file, tokens, output, index);
      break;
    case TOKEN_LPAREN:
      output = parse_call(file, tokens, index, output);
      break;
    default:
      passed = true;
    }
  }

  return output;
}

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag) {
  size_t index = 0;
  while (index < tokens->size) {
    ParsedValue *parsed_code = parse_token(file, tokens, &index, inline_flag);
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
  case AST_DECLARATION:
    free_declaration(parsed);
    break;
  case AST_CALL:
    free_parse_call(parsed);
    break;
  case AST_NUMBER:
  case AST_NULL:
  case AST_BOOLEAN:
    break;
  case AST_IF:
    free_parsed_if(parsed);
    break;
  }
}