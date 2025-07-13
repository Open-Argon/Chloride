/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../hash_data/hash_data.h"
#include "../../hashmap/hashmap.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../function/function.h"
#include "../literals/literals.h"
#include "../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_declaration(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  Token *token = darray_get(tokens, *index);

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_DECLARATION;
  DArray *declarations = checked_malloc(sizeof(DArray));
  darray_init(declarations, sizeof(ParsedSingleDeclaration));
  parsedValue->data = declarations;
  while (true) {
    ParsedSingleDeclaration _declaration = {};
    darray_push(declarations, &_declaration);
    ParsedSingleDeclaration *declaration =
        darray_get(declarations, declarations->size - 1);
    declaration->line = token->line;
    declaration->column = token->column;
    bool isFunction = false;
    DArray parameters;
    declaration->from = parse_null();

    if (token->type != TOKEN_IDENTIFIER) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){
          create_err(token->line, token->column, token->length, file,
                     "Syntax Error", "declaration requires an identifier"),
          NULL};
    }
    declaration->name =
        strcpy(checked_malloc(strlen(token->value) + 1), token->value);

    (*index)++;
    if ((*index) >= tokens->size)
      return (ParsedValueReturn){no_err, parsedValue};
    token = darray_get(tokens, *index);
    if (token->type == TOKEN_LPAREN) {
      isFunction = true;
      struct hashmap *parameters_hashmap = createHashmap();
      darray_init(&parameters, sizeof(char *));
      (*index)++;
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        darray_free(&parameters, free_parameter);
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RPAREN) {
        (*index)++;
        if ((*index) >= tokens->size) {
          declaration->from = create_parsed_function(
              declaration->name, parameters, declaration->from);
          return (ParsedValueReturn){no_err, parsedValue};
        }
        token = darray_get(tokens, *index);
      } else {
        while ((*index) < tokens->size) {
          skip_newlines_and_indents(tokens, index);
          ArErr err = error_if_finished(file, tokens, index);
          if (err.exists) {
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){err, NULL};
          }
          token = darray_get(tokens, *index);
          if (token->type != TOKEN_IDENTIFIER) {
            darray_free(&parameters, free_parameter);
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){
                create_err(
                    token->line, token->column, token->length, file,
                    "Syntax Error",
                    "parameter names need to start with a letter or _, only "
                    "use letters, digits, or _, and can't be keywords."),
                NULL};
          }
          uint64_t hash =
              siphash64_bytes(token->value, token->length, siphash_key);
          if (hashmap_lookup(parameters_hashmap, hash) != NULL) {
            darray_free(&parameters, free_parameter);
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){
                create_err(token->line, token->column, token->length, file,
                           "Syntax Error",
                           "duplicate argument in function "
                           "definition"),
                NULL};
          }
          char *parameter_name = checked_malloc(token->length + 1);
          strcpy(parameter_name, token->value);
          hashmap_insert(parameters_hashmap, hash, parameter_name, (void *)1,
                         0);
          darray_push(&parameters, &parameter_name);
          (*index)++;
          err = error_if_finished(file, tokens, index);
          if (err.exists) {
            darray_free(&parameters, free_parameter);
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){err, NULL};
          }
          skip_newlines_and_indents(tokens, index);
          err = error_if_finished(file, tokens, index);
          if (err.exists) {
            darray_free(&parameters, free_parameter);
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){err, NULL};
          }
          token = darray_get(tokens, *index);
          if (token->type == TOKEN_RPAREN) {
            (*index)++;
            if ((*index) >= tokens->size)
              return (ParsedValueReturn){no_err, parsedValue};
            token = darray_get(tokens, *index);
            break;
          } else if (token->type != TOKEN_COMMA) {
            darray_free(&parameters, free_parameter);
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){
                create_err(token->line, token->column, token->length, file,
                           "Syntax Error", "expected comma"),
                NULL};
          }
          (*index)++;
          err = error_if_finished(file, tokens, index);
          if (err.exists) {
            darray_free(&parameters, free_parameter);
            free_parsed(parsedValue);
            free(parsedValue);
            return (ParsedValueReturn){err, NULL};
          }
        }
      }
      hashmap_free(parameters_hashmap, NULL);
    }
    if (token->type == TOKEN_ASSIGN) {
      (*index)++;
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }

      ParsedValueReturn from = parse_token(file, tokens, index, true);
      if (from.err.exists) {
        darray_free(&parameters, free_parameter);
        free_parsed(parsedValue);
        free(parsedValue);
        return from;
      }
      free(declaration->from);
      declaration->from = from.value;
      if (!declaration->from) {
        darray_free(&parameters, free_parameter);
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected body"),
                                   NULL};
      }
    }
    if (isFunction) {
      declaration->from = create_parsed_function(declaration->name, parameters,
                                                 declaration->from);
    }
    if ((*index) >= tokens->size)
      break;
    token = darray_get(tokens, *index);

    size_t count = skip_newlines_and_indents(tokens, index);
    if ((*index) >= tokens->size)
      break;
    token = darray_get(tokens, *index);
    if (token->type != TOKEN_COMMA) {
      (*index) -= count;
      break;
    }
    (*index)++;
    ArErr err = error_if_finished(file, tokens, index);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    token = darray_get(tokens, *index);
  }
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_string(void *ptr) {
  // `ptr` is a pointer to a char*
  char *str = *(char **)ptr;
  free(str);
}

void free_single_declaration(void *ptr) {
  ParsedSingleDeclaration *declaration = ptr;
  if (declaration->name)
    free(declaration->name);
  if (declaration->from) {
    free_parsed(declaration->from);
    free(declaration->from);
  }
}

void free_declaration(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *declaration = parsedValue->data;
  darray_free(declaration, free_single_declaration);
  free(declaration);
}