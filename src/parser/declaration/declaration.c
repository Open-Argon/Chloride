/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../err.h"
#include "../../hash_data/hash_data.h"
#include "../../hashmap/hashmap.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../function/function.h"
#include "param_list.h"
#include "../literals/literals.h"
#include "../parser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define setStage(n)                                                            \
  if (stage < n)                                                               \
  stage = n
// ── Parameter parsing state ──────────────────────────────────────────────────

void param_state_init(ParamState *ps) {
  ps->seen = createHashmap();
  darray_init(&ps->positional, sizeof(char *));
  ps->defaults = NULL;
  ps->v_param = NULL;
  ps->kw_param = NULL;
  ps->stage = 0;
}

void param_state_free(ParamState *ps) {
  if (ps->seen)
    hashmap_free(ps->seen, NULL);
  darray_free(&ps->positional, free_parameter);
  if (ps->defaults) {
    darray_free(ps->defaults, free_default_value_parameter);
    free(ps->defaults);
  }
  if (ps->v_param)
    free(ps->v_param);
  if (ps->kw_param)
    free(ps->kw_param);
}

// ── Single-declaration cleanup helper ───────────────────────────────────────

static ParsedValueReturn decl_err(ParsedValue *parsedValue,
                                  ParamState *ps, // NULL if not in param parse
                                  ArErr err) {
  if (ps)
    param_state_free(ps);
  if (parsedValue) {
    free_parsed(parsedValue);
    free(parsedValue);
  }
  return (ParsedValueReturn){err, NULL};
}

// ── Parameter list parser ────────────────────────────────────────────────────

// Returns no_err on success (ps is populated).
// Returns an error and leaves ps partially populated (caller must still call
// param_state_free).
ArErr parse_param_list(char *file, DArray *tokens, size_t *index,
                              ParamState *ps) {
  // index is already pointing past the TOKEN_LPAREN
  ArErr err;

#define PARAM_ERR(msg)                                                         \
  path_specific_create_err(token->line, token->column, token->length, file,    \
                           SyntaxError, (msg))

  Token *token;

  while (*index < tokens->size) {
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err))
      return err;
    token = darray_get(tokens, *index);

    // ── closing paren ────────────────────────────────────────────────────
    if (token->type == TOKEN_RPAREN) {
      (*index)++;
      return no_err;
    }

    // ── **kwargs ─────────────────────────────────────────────────────────
    if (token->type == TOKEN_STAR) {
      (*index)++;
      err = error_if_finished(file, tokens, index);
      if (is_error(&err))
        return err;
      token = darray_get(tokens, *index);

      if (token->type == TOKEN_STAR) {
        // ** — kwargs
        if (ps->kw_param)
          return PARAM_ERR("only one **kwargs parameter is allowed");
        (*index)++;
        err = error_if_finished(file, tokens, index);
        if (is_error(&err))
          return err;
        token = darray_get(tokens, *index);
        if (token->type != TOKEN_IDENTIFIER)
          return PARAM_ERR("expected identifier after **");
        ps->kw_param = strcpy(checked_malloc(token->length + 1), token->value);
        ps->stage = 3;
      } else {
        // single * — *args
        if (ps->stage >= 2)
          return PARAM_ERR("only one *args parameter is allowed");
        if (token->type != TOKEN_IDENTIFIER)
          return PARAM_ERR("expected identifier after *");
        ps->v_param = strcpy(checked_malloc(token->length + 1), token->value);
        ps->stage = 2;
      }

      (*index)++;
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err))
        return err;
      token = darray_get(tokens, *index);

      if (token->type == TOKEN_RPAREN) {
        (*index)++;
        return no_err;
      }
      if (token->type != TOKEN_COMMA)
        return PARAM_ERR("expected comma or ) after *args/**kwargs");
      (*index)++;
      continue;
    }

    // ── must be a normal identifier ──────────────────────────────────────
    if (token->type != TOKEN_IDENTIFIER)
      return PARAM_ERR(
          "parameter names need to start with a letter or _, "
          "only use letters, digits, or _, and can't be keywords.");

    if (ps->stage == 3)
      return PARAM_ERR("no parameters allowed after **kwargs");

    // duplicate check
    uint64_t hash =
        siphash64_bytes(token->value, token->length, siphash_key_fixed);
    if (hashmap_lookup(ps->seen, hash) != NULL)
      return PARAM_ERR("duplicate argument in function definition");

    char *name = strcpy(checked_malloc(token->length + 1), token->value);
    hashmap_insert(ps->seen, hash, name, (void *)1, 0);

    (*index)++;
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      free(name);
      return err;
    }
    token = darray_get(tokens, *index);

    // ── optional default value ───────────────────────────────────────────
    if (token->type == TOKEN_QUESTION) {
      if (ps->stage >= 2)
        return PARAM_ERR("optional parameter not allowed after *args/**kwargs");
      ps->stage = 1;
      if (!ps->defaults) {
        ps->defaults = checked_malloc(sizeof(DArray));
        darray_init(ps->defaults, sizeof(struct default_value_parameter));
      }
      struct default_value_parameter entry = {name, parse_null()};
      darray_push(ps->defaults, &entry);
      (*index)++;
      token = darray_get(tokens, *index); // ← this was missing
    } else if (token->type == TOKEN_ASSIGN) {
      if (ps->stage == 2)
        return PARAM_ERR("*args parameter cannot have a default value");
      ps->stage = 1;

      (*index)++;
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        free(name);
        return err;
      }

      ParsedValueReturn dv = parse_token(file, tokens, index, true);
      if (is_error(&dv.err)) {
        free(name);
        return dv.err;
      }
      if (!dv.value) {
        free(name);
        return PARAM_ERR("expected value");
      }

      if (!ps->defaults) {
        ps->defaults = checked_malloc(sizeof(DArray));
        darray_init(ps->defaults, sizeof(struct default_value_parameter));
      }
      struct default_value_parameter entry = {name, dv.value};
      darray_push(ps->defaults, &entry);

      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err))
        return err;
      token = darray_get(tokens, *index);
    } else {
      if (ps->stage == 1) {
        free(name);
        return PARAM_ERR("non-default argument follows default argument");
      }
      if (ps->stage >= 2) {
        free(name);
        return PARAM_ERR("no positional parameters allowed after *args");
      }
      darray_push(&ps->positional, &name);
    }

    // ── comma or closing paren ───────────────────────────────────────────
    if (token->type == TOKEN_RPAREN) {
      (*index)++;
      return no_err;
    }
    if (token->type != TOKEN_COMMA)
      return PARAM_ERR("expected comma");
    (*index)++;

    err = error_if_finished(file, tokens, index);
    if (is_error(&err))
      return err;
  }

  // ran off the end
  return path_specific_create_err(0, 0, 0, file, SyntaxError,
                                  "unexpected end of file in parameter list");
#undef PARAM_ERR
}

// ── Main declaration parser ──────────────────────────────────────────────────

ParsedValueReturn parse_declaration(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err))
    return (ParsedValueReturn){err, NULL};

  Token *token = darray_get(tokens, *index);

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_DECLARATION;
  DArray *declarations = checked_malloc(sizeof(DArray));
  darray_init(declarations, sizeof(ParsedSingleDeclaration));
  parsedValue->data = declarations;

  while (true) {
    // ── push a fresh declaration slot ────────────────────────────────────
    ParsedSingleDeclaration _decl = {};
    darray_push(declarations, &_decl);
    ParsedSingleDeclaration *decl =
        darray_get(declarations, declarations->size - 1);
    decl->line = token->line;
    decl->column = token->column;
    decl->from = parse_null();

    bool isFunction = false;
    ParamState ps;

    // ── name ─────────────────────────────────────────────────────────────
    if (token->type != TOKEN_IDENTIFIER) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   file, SyntaxError,
                                   "declaration requires an identifier"),
          NULL};
    }
    decl->name = strcpy(checked_malloc(strlen(token->value) + 1), token->value);

    (*index)++;
    if (*index >= tokens->size)
      return (ParsedValueReturn){no_err, parsedValue};
    token = darray_get(tokens, *index);

    // ── optional parameter list ──────────────────────────────────────────
    if (token->type == TOKEN_LPAREN) {
      isFunction = true;
      param_state_init(&ps);
      (*index)++;

      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        param_state_free(&ps);
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }

      token = darray_get(tokens, *index);

      // empty param list fast-path
      if (token->type == TOKEN_RPAREN) {
        (*index)++;
        if (*index >= tokens->size) {
          decl->from =
              create_parsed_function(decl->name, ps.positional, ps.defaults,
                                     ps.v_param, ps.kw_param, decl->from);
          hashmap_free(ps.seen, NULL); // ← add this before the return
          return (ParsedValueReturn){no_err, parsedValue};
        }
        hashmap_free(ps.seen, NULL); // ← add this here too
        token = darray_get(tokens, *index);
      } else {
        err = parse_param_list(file, tokens, index, &ps);
        if (is_error(&err)) {
          param_state_free(&ps);
          free_parsed(parsedValue);
          free(parsedValue);
          return (ParsedValueReturn){err, NULL};
        }
        if (*index < tokens->size)
          token = darray_get(tokens, *index);
      }

      // ps.seen no longer needed
      hashmap_free(ps.seen, NULL);
      ps.seen = NULL;
    }

    // ── = body ───────────────────────────────────────────────────────────
    if (token->type == TOKEN_ASSIGN) {
      (*index)++;
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        if (isFunction)
          param_state_free(&ps);
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }

      ParsedValueReturn body = parse_token(file, tokens, index, true);
      if (is_error(&body.err)) {
        if (isFunction)
          param_state_free(&ps);
        free_parsed(parsedValue);
        free(parsedValue);
        return body;
      }
      if (!body.value) {
        if (isFunction)
          param_state_free(&ps);
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){
            path_specific_create_err(token->line, token->column, token->length,
                                     file, SyntaxError, "expected body"),
            NULL};
      }
      free(decl->from);
      decl->from = body.value;
    }

    if (isFunction) {
      decl->from =
          create_parsed_function(decl->name, ps.positional, ps.defaults,
                                 ps.v_param, ps.kw_param, decl->from);
      if (ps.seen)
        hashmap_free(ps.seen, NULL);
    }

    // ── check for comma-separated declarations ───────────────────────────
    if (*index >= tokens->size)
      break;
    token = darray_get(tokens, *index);

    size_t count = skip_newlines_and_indents(tokens, index);
    if (*index >= tokens->size)
      break;
    token = darray_get(tokens, *index);

    if (token->type != TOKEN_COMMA) {
      (*index) -= count;
      break;
    }
    (*index)++;

    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
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