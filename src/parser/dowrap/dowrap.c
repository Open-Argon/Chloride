#include "dowrap.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *repeat_space(size_t x) {

  char *str = checked_malloc(x + 1); // +1 for the null terminator

  memset(str, ' ', x);
  str[x] = '\0';

  return str;
}

void free_string_dowrap(void *ptr) {
  // `ptr` is a pointer to a char*
  char *str = *(char **)ptr;
  free(str);
}

ParsedValue *parse_dowrap(char *file, DArray *tokens, size_t *index) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_DOWRAP;
  DArray *dowrap_parsed = checked_malloc(sizeof(DArray));
  darray_init(dowrap_parsed, sizeof(ParsedValue));
  parsedValue->data = dowrap_parsed;
  (*index)++;
  if ((*index) >= tokens->size)
    return parsedValue;
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_NEW_LINE) {
    fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  }
  size_t indent_depth = 0;
  bool temp_indent_depth_toggle = false;
  size_t temp_indent_depth = 0;
  bool pass = false;
  DArray dowrap_tokens;
  darray_init(&dowrap_tokens, sizeof(Token));
  DArray to_free;
  darray_init(&to_free, sizeof(char *));

  size_t starting_index = *index;

  size_t temp_index_count = 0;

  while (!pass && ++(*index) < tokens->size) {
    token = darray_get(tokens, *index);
    switch (token->type) {
    case TOKEN_INDENT:
      temp_indent_depth_toggle = true;
      if (dowrap_tokens.size == 0) {
        indent_depth = strlen(token->value);
        temp_indent_depth = indent_depth;
      } else {
        temp_indent_depth = strlen(token->value);
      }
      break;
    case TOKEN_NEW_LINE:
      temp_indent_depth = 0;
      temp_indent_depth_toggle = true;
      darray_push(&dowrap_tokens, token);
      temp_index_count++;
      break;
    default:
      if (temp_indent_depth < indent_depth && temp_indent_depth_toggle) {
        pass = true;
        break;
      }
      if (temp_indent_depth > indent_depth) {
        size_t indent_amount = temp_indent_depth-indent_depth;
        Token indent_token;
        indent_token.line = token->line;
        indent_token.column = token->column;
        indent_token.type = TOKEN_INDENT;
        indent_token.value = repeat_space(indent_amount);
        darray_push(&dowrap_tokens, &indent_token);
        darray_push(&to_free, &indent_token.value);
      }
      temp_indent_depth_toggle = false;
      temp_indent_depth = 0;
      temp_index_count=0;
      darray_push(&dowrap_tokens, token);
    }
  }
  (*index)-=temp_index_count;
  for (size_t i = 0; i<temp_index_count;i++) {
    darray_pop(&dowrap_tokens, NULL);
  }
  parser(file, dowrap_parsed, &dowrap_tokens, false);

  darray_free(&dowrap_tokens, NULL);

  darray_free(&to_free, free_string_dowrap);

  return parsedValue;
}

void free_dowrap(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed = parsedValue->data;
  darray_free(parsed, free_parsed);
  free(parsed);
}