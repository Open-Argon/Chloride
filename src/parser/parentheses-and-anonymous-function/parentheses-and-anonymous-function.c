/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "parentheses-and-anonymous-function.h"
#include "../../err.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../../hashmap/hashmap.h"
#include "../../lexer/token.h"
#include "../function/function.h"
#include "../declaration/param_list.h"
#include <stddef.h>
#include <string.h>

ParsedValueReturn parse_parentheses(char *file, DArray *tokens, size_t *index) {
  (*index)++; // consume (
  skip_newlines_and_indents(tokens, index);
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err))
    return (ParsedValueReturn){err, NULL};

  Token *token = darray_get(tokens, *index);

  // ── peek ahead to decide: anonymous function or parenthesised expression ──
  // It's an anonymous function if:
  //   ()        => empty params
  //   (*        => starts with *args
  //   (**       => starts with **kwargs
  //   ident =   => keyword default
  //   ident ?   => null-default shorthand
  //   ident ,   => multiple params
  //   ident )=  => single param followed by ) then =
  bool is_anon_fn = false;

  if (token->type == TOKEN_RPAREN) {
    // () — peek past ) for =
    size_t saved = *index;
    (*index)++; // skip )
    skip_newlines_and_indents(tokens, index);
    if (*index < tokens->size) {
      Token *next = darray_get(tokens, *index);
      if (next->type == TOKEN_ASSIGN)
        is_anon_fn = true;
    }
    *index = saved; // always restore, we handle both paths below
  } else if (token->type == TOKEN_STAR) {
    is_anon_fn = true;
  } else if (token->type == TOKEN_IDENTIFIER) {
    size_t saved = *index;
    (*index)++;
    if (*index < tokens->size) {
      Token *next = darray_get(tokens, *index);
      if (next->type == TOKEN_ASSIGN   ||
          next->type == TOKEN_QUESTION ||
          next->type == TOKEN_COMMA) {
        is_anon_fn = true;
      } else if (next->type == TOKEN_RPAREN) {
        // single ident then ) — peek further for =
        (*index)++;
        skip_newlines_and_indents(tokens, index);
        if (*index < tokens->size) {
          Token *after = darray_get(tokens, *index);
          if (after->type == TOKEN_ASSIGN)
            is_anon_fn = true;
        }
      }
    }
    *index = saved;
  }

  // ── anonymous function path ───────────────────────────────────────────────
  if (is_anon_fn) {
    ParamState ps;
    param_state_init(&ps);

    if (token->type == TOKEN_RPAREN) {
      (*index)++; // consume )
    } else {
      err = parse_param_list(file, tokens, index, &ps);
      if (is_error(&err)) {
        param_state_free(&ps);
        return (ParsedValueReturn){err, NULL};
      }
    }

    // consume =
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      param_state_free(&ps);
      return (ParsedValueReturn){err, NULL};
    }
    token = darray_get(tokens, *index);
    if (token->type != TOKEN_ASSIGN) {
      param_state_free(&ps);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   file, SyntaxError, "expected =>"),
          NULL};
    }
    (*index)++;
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      param_state_free(&ps);
      return (ParsedValueReturn){err, NULL};
    }

    ParsedValueReturn body = parse_token(file, tokens, index, true);
    if (is_error(&body.err)) {
      param_state_free(&ps);
      return body;
    }
    if (!body.value) {
      param_state_free(&ps);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   file, SyntaxError, "expected body"),
          NULL};
    }

    // transfer ownership — same pattern as declaration.c
    if (ps.seen) { hashmap_free(ps.seen, NULL); ps.seen = NULL; }
    return (ParsedValueReturn){
        no_err, create_parsed_function("anonymous", ps.positional, ps.defaults,
                                       ps.v_param, ps.kw_param, body.value)};
  }

  // ── parenthesised expression path ────────────────────────────────────────
  DArray list;
  darray_init(&list, sizeof(ParsedValue));

  if (token->type != TOKEN_RPAREN) {
    while (*index < tokens->size) {
      ParsedValueReturn parsedItem = parse_token(file, tokens, index, true);
      if (is_error(&parsedItem.err)) {
        darray_free(&list, (void (*)(void *))free_parsed);
        return parsedItem;
      }
      darray_push(&list, parsedItem.value);
      free(parsedItem.value);

      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        darray_free(&list, (void (*)(void *))free_parsed);
        return (ParsedValueReturn){err, NULL};
      }
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RPAREN) {
        break;
      } else if (token->type != TOKEN_COMMA) {
        darray_free(&list, (void (*)(void *))free_parsed);
        return (ParsedValueReturn){
            path_specific_create_err(token->line, token->column, token->length,
                                     file, SyntaxError, "expected comma"),
            NULL};
      }
      (*index)++;
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        darray_free(&list, (void (*)(void *))free_parsed);
        return (ParsedValueReturn){err, NULL};
      }
    }
  }
  (*index)++; // consume )

  if (list.size != 1) {
    token = (*index < tokens->size) ? darray_get(tokens, *index) : token;
    darray_free(&list, (void (*)(void *))free_parsed);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected 1 body"),
        NULL};
  }

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  memcpy(parsedValue, darray_get(&list, 0), sizeof(ParsedValue));
  darray_free(&list, NULL);
  return (ParsedValueReturn){no_err, parsedValue};
}