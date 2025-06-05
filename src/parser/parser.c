#include "parser.h"
#include "../dynamic_array/darray.h"
#include "../lexer/token.h"
#include "assignable/access/access.h"
#include "assignable/assign/assign.h"
#include "assignable/call/call.h"
#include "assignable/identifier/identifier.h"
#include "declaration/declaration.h"
#include "dowrap/dowrap.h"
#include "if/if.h"
#include "literals/literals.h"
#include "number/number.h"
#include "string/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ValueTypeNames[] = {
    "string", "assign",      "identifier", "number",  "if statement", "access",
    "call",   "declaration", "null",       "boolean", "do wrap"};

void error_if_finished(char *file, DArray *tokens, size_t *index) {
  if ((*index) >= tokens->size) {
    Token *token = darray_get(tokens, tokens->size - 1);
    fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  }
}

void skip_newlines_and_indents(DArray *tokens, size_t *index) {
  bool passed = false;
  while (!passed && (*index) < tokens->size) {
    Token *token = darray_get(tokens, *index);
    switch (token->type) {
    case TOKEN_NEW_LINE:
    case TOKEN_INDENT:
      (*index)++;
      break;
    default:
      passed = true;
    }
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
    (*index)++;
    return NULL;
  case TOKEN_INDENT:
    if (strlen(token->value) > 0) {
    fprintf(stderr, "%s:%zu:%zu error: invalid indentation\n", file,
            token->line, token->column);
    exit(EXIT_FAILURE);
    }
    (*index)++;
    return NULL;
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
  case TOKEN_DO:
    output = parse_dowrap(file, tokens, index);
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
    case TOKEN_ASSIGN_STAR:
      output = parse_assign(file, tokens, output, index);
      break;
    case TOKEN_LPAREN:
      output = parse_call(file, tokens, index, output);
      break;
    case TOKEN_DOT:
      output = parse_access(file, tokens, index, output);
      break;
    default:
      passed = true;
    }
  }

  return output;
}

void parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag) {
  size_t index = 0;
  bool expecting_new_line = false;
  while (index < tokens->size) {
    ParsedValue *parsed_code = parse_token(file, tokens, &index, inline_flag);
    if (parsed_code) {
      if (expecting_new_line) {
        Token *token = darray_get(tokens, index-1);
        fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
                token->column);
        exit(EXIT_FAILURE);
      }
      expecting_new_line = true;
      darray_push(parsed, parsed_code);
      free(parsed_code);
    } else {
      expecting_new_line = false;
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
  case AST_ACCESS:
    free_parse_access(parsed);
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