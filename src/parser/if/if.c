/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "if.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void free_conditional(void *ptr) {
  ParsedConditional *conditional = ptr;
  if (conditional->condition) {
    free_parsed(conditional->condition);
    free(conditional->condition);
  }
  free_parsed(conditional->content);
  free(conditional->content);
}

ParsedValueReturn parse_if(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }

  DArray *parsed_if = checked_malloc(sizeof(DArray));
  darray_init(parsed_if, sizeof(ParsedConditional));

  bool expect_conditional = true;

  while (*index < tokens->size) {
    Token *token = darray_get(tokens, *index);

    // Handle TOKEN_ELSE or TOKEN_ELSE_IF for subsequent branches
    if (!expect_conditional) {
      if (token->type != TOKEN_NEW_LINE)
        break; // no more branches
      size_t current_index = *index;
      skip_newlines_and_indents(tokens, index);
      if ((*index) >= tokens->size)
        break;
      token = darray_get(tokens, *index);

      if (token->type == TOKEN_ELSE || token->type == TOKEN_ELSE_IF) {
        (*index)++;
        ArErr err = error_if_finished(file, tokens, index);
        if (err.exists) {
          darray_free(parsed_if, free_conditional);
          free(parsed_if);
          return (ParsedValueReturn){err, NULL};
        }
      } else {
        *index = current_index;
        break; // no more branches
      }
    }

    ParsedValueReturn condition = {no_err, NULL};

    if (token->type != TOKEN_ELSE) {
      // Parse ( condition )
      token = darray_get(tokens, *index);
      if (token->type != TOKEN_LPAREN) {
        darray_free(parsed_if, free_conditional);
        free(parsed_if);
        return (ParsedValueReturn){
            create_err(token->line, token->column, token->length, file,
                       "Syntax Error", "expected '(' after if"),
            NULL};
      }

      (*index)++;
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        darray_free(parsed_if, free_conditional);
        free(parsed_if);
        return (ParsedValueReturn){err, NULL};
      }
      skip_newlines_and_indents(tokens, index);
      condition = parse_token(file, tokens, index, true);
      if (condition.err.exists) {
        darray_free(parsed_if, free_conditional);
        free(parsed_if);
        return condition;
      } else if (!condition.value) {
        darray_free(parsed_if, free_conditional);
        free(parsed_if);
        return (ParsedValueReturn){
            create_err(token->line, token->column, token->length, file,
                       "Syntax Error", "expected condition"),
            NULL};
      }
      skip_newlines_and_indents(tokens, index);

      token = darray_get(tokens, *index);
      if (token->type != TOKEN_RPAREN) {
        if (condition.value) {
          free_parsed(condition.value);
          free(condition.value);
        }
        darray_free(parsed_if, free_conditional);
        free(parsed_if);
        return (ParsedValueReturn){
            create_err(token->line, token->column, token->length, file,
                       "Syntax Error", "missing closing ')' in condition"),
            NULL};
      }

      (*index)++;
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        if (condition.value) {
          free_parsed(condition.value);
          free(condition.value);
        }
        darray_free(parsed_if, free_conditional);
        free(parsed_if);
        return (ParsedValueReturn){err, NULL};
      }
    }

    // Parse the body
    ParsedValueReturn parsed_content = parse_token(file, tokens, index, false);

    if (parsed_content.err.exists) {
      if (condition.value) {
        free_parsed(condition.value);
        free(condition.value);
      }
      darray_free(parsed_if, free_conditional);
      free(parsed_if);
      return parsed_content;
    }

    if (!parsed_content.value) {
      if (condition.value) {
        free_parsed(condition.value);
        free(condition.value);
      }
      darray_free(parsed_if, free_conditional);
      free(parsed_if);
      return (ParsedValueReturn){create_err(token->line, token->column,
                                            token->length, file, "Syntax Error",
                                            "expected body"),
                                 NULL};
    }

    ParsedConditional conditional = {condition.value, parsed_content.value};
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

  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_if(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed_if = parsedValue->data;
  darray_free(parsed_if, free_conditional);
  free(parsed_if);
}