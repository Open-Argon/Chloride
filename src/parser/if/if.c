#include "if.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValue *parse_if(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  error_if_finished(file, tokens, index);

  DArray *parsed_if = checked_malloc(sizeof(DArray));
  darray_init(parsed_if, sizeof(ParsedConditional));

  bool expect_conditional = true;

  while (*index < tokens->size) {
    Token *token = darray_get(tokens, *index);

    // Handle TOKEN_ELSE or TOKEN_ELSE_IF for subsequent branches
    if (!expect_conditional) {
      if (token->type != TOKEN_NEW_LINE)
        break; // no more branches
      (*index)++;
      if ((*index) >= tokens->size)
        break;
      token = darray_get(tokens, *index);

      if (token->type == TOKEN_ELSE || token->type == TOKEN_ELSE_IF) {
        (*index)++;
        error_if_finished(file, tokens, index);
      } else {
        break; // no more branches
      }
    }

    DArray *condition = NULL;

    if (token->type != TOKEN_ELSE) {
      // Parse ( condition )
      token = darray_get(tokens, *index);
      if (token->type != TOKEN_LPAREN) {
        fprintf(stderr, "%s:%zu:%zu error: expected '(' after if\n", file,
                token->line, token->column);
        exit(EXIT_FAILURE);
      }

      (*index)++;
      error_if_finished(file, tokens, index);

      condition = checked_malloc(sizeof(DArray));
      darray_init(condition, sizeof(ParsedValue));

      while (*index < tokens->size) {
        ParsedValue *parsed_code = parse_token(file, tokens, index, true);
        if (parsed_code) {
          darray_push(condition, parsed_code);
          free(parsed_code);
        }

        token = darray_get(tokens, *index);
        if (token->type == TOKEN_RPAREN)
          break;
      }

      if (token->type != TOKEN_RPAREN) {
        fprintf(stderr, "%s:%zu:%zu error: missing closing ')' in condition\n",
                file, token->line, token->column);
        exit(EXIT_FAILURE);
      }

      (*index)++;
      error_if_finished(file, tokens, index);
    }

    // Parse the body
    ParsedValue *parsed_content = parse_token(file, tokens, index, false);

    if (!parsed_content) {
      fprintf(stderr, "%s:%zu:%zu error: expected body after condition\n", file,
              token->line, token->column);
      exit(EXIT_FAILURE);
    }

    ParsedConditional conditional = {condition, parsed_content};
    darray_push(parsed_if, &conditional);

    expect_conditional =
        false; // After first iteration, expect newline + else/else if
  }

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_IF;
  parsedValue->data = parsed_if;

  // printf("Parsed if chain:\n");
  // for (size_t i = 0; i < parsed_if->size; i++) {
  //   ParsedConditional *cond = darray_get(parsed_if, i);
  //   if (cond->condition) {
  //     printf("  if/else if condition:\n");
  //     for (size_t j = 0; j < cond->condition->size; j++) {
  //       ParsedValue *v = darray_get(cond->condition, j);
  //       printf("    - condition value type: %d\n", v->type);
  //     }
  //   } else {
  //     printf("  else:\n");
  //   }
  //   printf("    - content value type: %d\n", cond->content->type);
  // }

  return parsedValue;
}

void free_conditional(void *ptr) {
  ParsedConditional *conditional = ptr;
  if (conditional->condition)
    darray_free(conditional->condition, free_parsed);
  free_parsed(conditional->content);
  free(conditional->content);
}

void free_parsed_if(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed_if = parsedValue->data;
  darray_free(parsed_if, free_conditional);
}