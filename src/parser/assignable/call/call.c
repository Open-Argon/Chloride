/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "call.h"
#include "../../../err.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../../runtime/objects/exceptions/exceptions.h"
#include "../../function/function.h"
#include "../../literals/literals.h"
#include "../../parser.h"
#include <stdlib.h>
#include <string.h>

// ── Call argument state
// ───────────────────────────────────────────────────────

typedef struct {
  DArray args;    // positional ParsedValue
  DArray *kwargs; // struct parsed_kwarg, lazy-allocated
  char *v_arg;    // *ident or NULL
  char *kw_arg;   // **ident or NULL
  uint8_t stage;  // 0=positional 1=kwargs 2=*arg 3=**arg
  bool must_assign;
} CallArgState;

static void call_arg_state_init(CallArgState *cs) {
  darray_init(&cs->args, sizeof(ParsedValue));
  cs->kwargs = NULL;
  cs->v_arg = NULL;
  cs->kw_arg = NULL;
  cs->stage = 0;
  cs->must_assign = false;
}

static void call_arg_state_free(CallArgState *cs) {
  darray_free(&cs->args, (void (*)(void *))free_parsed);
  if (cs->kwargs) {
    darray_free(cs->kwargs, free_default_value_parameter);
    free(cs->kwargs);
  }
  if (cs->v_arg)
    free(cs->v_arg);
  if (cs->kw_arg)
    free(cs->kw_arg);
}

// ── Arg list parser
// ───────────────────────────────────────────────────────────

static ArErr parse_arg_list(char *file, DArray *tokens, size_t *index,
                            CallArgState *cs) {
  ArErr err;

#define ARG_ERR(msg)                                                           \
  path_specific_create_err(token->line, token->column, token->length, file,    \
                           SyntaxError, (msg))

  Token *token;

  while (*index < tokens->size) {
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err))
      return err;
    token = darray_get(tokens, *index);

    // ── closing paren ─────────────────────────────────────────────────────
    if (token->type == TOKEN_RPAREN) {
      (*index)++;
      return no_err;
    }

    // ── *ident or **ident ─────────────────────────────────────────────────
    if (token->type == TOKEN_STAR) {
      cs->must_assign = true;
      (*index)++;
      err = error_if_finished(file, tokens, index);
      if (is_error(&err))
        return err;
      token = darray_get(tokens, *index);

      if (token->type == TOKEN_STAR) {
        // **ident
        if (cs->kw_arg)
          return ARG_ERR("only one **ident argument is allowed");
        (*index)++;
        err = error_if_finished(file, tokens, index);
        if (is_error(&err))
          return err;
        token = darray_get(tokens, *index);
        if (token->type != TOKEN_IDENTIFIER)
          return ARG_ERR("expected identifier after **");
        cs->kw_arg = strcpy(checked_malloc(token->length + 1), token->value);
        cs->stage = 3;
      } else {
        // *ident
        if (cs->stage >= 2)
          return ARG_ERR("only one *ident argument is allowed");
        if (token->type != TOKEN_IDENTIFIER)
          return ARG_ERR("expected identifier after *");
        cs->v_arg = strcpy(checked_malloc(token->length + 1), token->value);
        cs->stage = 2;
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
        return ARG_ERR("expected comma or ) after *ident/**ident");
      (*index)++;
      continue;
    }

    // ── peek ahead for name= (keyword arg) ───────────────────────────────
    // We need to check if this is `identifier=` without consuming it yet.
    // Only do this if the current token is an identifier.
    bool is_kwarg = false;
    if (token->type == TOKEN_IDENTIFIER && cs->stage < 2) {
      size_t saved = *index;
      (*index)++;
      if (*index < tokens->size) {
        Token *next = darray_get(tokens, *index);
        if (next->type == TOKEN_ASSIGN) {
          is_kwarg = true;
        }
      }
      if (!is_kwarg)
        *index = saved; // restore if not a kwarg
    }

    if (is_kwarg) {
      if (cs->stage == 3)
        return ARG_ERR("no arguments allowed after **ident");
      cs->stage = 1;

      // token is still the identifier (index is on the = now)
      Token *name_token = darray_get(tokens, *index - 1);
      char *name =
          strcpy(checked_malloc(name_token->length + 1), name_token->value);

      (*index)++; // skip =
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        free(name);
        return err;
      }

      ParsedValueReturn val = parse_token(file, tokens, index, true);
      if (is_error(&val.err)) {
        free(name);
        return val.err;
      }
      if (!val.value) {
        free(name);
        return ARG_ERR("expected value");
      }

      if (!cs->kwargs) {
        cs->kwargs = checked_malloc(sizeof(DArray));
        darray_init(cs->kwargs, sizeof(struct default_value_parameter));
      }
      struct default_value_parameter entry = {name, val.value};
      darray_push(cs->kwargs, &entry);

      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err))
        return err;
      token = darray_get(tokens, *index);

    } else {
      // ── check for `x?` shorthand (sugar for x=null kwarg) ────────────
      bool is_null_shorthand = false;
      char *null_shorthand_name = NULL;
      if (token->type == TOKEN_IDENTIFIER) {
        size_t saved = *index;
        Token *ident_token = token;
        (*index)++;
        if (*index < tokens->size) {
          Token *next = darray_get(tokens, *index);
          if (next->type == TOKEN_QUESTION) {
            is_null_shorthand = true;
            null_shorthand_name = strcpy(
                checked_malloc(ident_token->length + 1), ident_token->value);
            (*index)++; // consume ?
            cs->must_assign = true;
          }
        }
        if (!is_null_shorthand)
          *index = saved;
      }

      if (is_null_shorthand) {
        // x? is identical to x=null — treat as a kwarg
        if (cs->stage == 3) {
          free(null_shorthand_name);
          return ARG_ERR("no arguments allowed after **ident");
        }
        cs->stage = 1;

        if (!cs->kwargs) {
          cs->kwargs = checked_malloc(sizeof(DArray));
          darray_init(cs->kwargs, sizeof(struct default_value_parameter));
        }
        ParsedValue *null_val = parse_null();
        struct default_value_parameter entry = {null_shorthand_name, null_val};
        darray_push(cs->kwargs, &entry);

        skip_newlines_and_indents(tokens, index);
        err = error_if_finished(file, tokens, index);
        if (is_error(&err))
          return err;
        token = darray_get(tokens, *index);

      } else {
        // ── normal positional arg ─────────────────────────────────────
        if (cs->stage == 1)
          return ARG_ERR("positional argument follows keyword argument");
        if (cs->stage == 3)
          return ARG_ERR("no arguments allowed after **ident");

        ParsedValueReturn val = parse_token(file, tokens, index, true);
        if (is_error(&val.err))
          return val.err;
        if (!val.value)
          return ARG_ERR("expected argument");

        darray_push(&cs->args, val.value);
        free(val.value);

        skip_newlines_and_indents(tokens, index);
        err = error_if_finished(file, tokens, index);
        if (is_error(&err))
          return err;
        token = darray_get(tokens, *index);
      }
    }

    // ── comma or closing paren ────────────────────────────────────────────
    if (token->type == TOKEN_RPAREN) {
      (*index)++;
      return no_err;
    }
    if (token->type != TOKEN_COMMA)
      return ARG_ERR("expected comma");
    (*index)++;

    err = error_if_finished(file, tokens, index);
    if (is_error(&err))
      return err;
  }

  return path_specific_create_err(0, 0, 0, file, SyntaxError,
                                  "unexpected end of file in argument list");
#undef ARG_ERR
}

// ── Public parse_call
// ─────────────────────────────────────────────────────────

ParsedValueReturn parse_call(char *file, DArray *tokens, size_t *index,
                             ParsedValue *to_call) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedCall *call = checked_malloc(sizeof(ParsedCall));
  Token *token = darray_get(tokens, *index);

  call->line = token->line;
  call->column = token->column;
  call->to_call = to_call;
  call->kwargs = NULL;
  call->v_arg = NULL;
  call->kw_arg = NULL;
  call->must_assign = false;
  call->args.data = NULL;
  parsedValue->data = call;
  parsedValue->type = AST_CALL;

  CallArgState cs;
  call_arg_state_init(&cs);

  (*index)++; // skip (
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    call_arg_state_free(&cs);
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){err, NULL};
  }

  token = darray_get(tokens, *index);

  if (token->type != TOKEN_RPAREN) {
    err = parse_arg_list(file, tokens, index, &cs);
    if (is_error(&err)) {
      call_arg_state_free(&cs);
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
  } else {
    (*index)++; // consume the )
  }

  // transfer ownership from cs into call
  call->args = cs.args;
  call->kwargs = cs.kwargs;
  call->v_arg = cs.v_arg;
  call->kw_arg = cs.kw_arg;
  call->must_assign = cs.must_assign;

  return (ParsedValueReturn){no_err, parsedValue};
}

// ── Cleanup
// ───────────────────────────────────────────────────────────────────

void free_parse_call(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedCall *call = parsedValue->data;

  if (call->args.data)
    darray_free(&call->args, (void (*)(void *))free_parsed);
  if (call->kwargs) {
    darray_free(call->kwargs, free_default_value_parameter);
    free(call->kwargs);
  }
  if (call->v_arg)
    free(call->v_arg);
  if (call->kw_arg)
    free(call->kw_arg);
  free_parsed(call->to_call);
  free(call->to_call);
  free(call);
}