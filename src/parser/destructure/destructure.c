/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "destructure.h"
#include "../../dynamic_array/darray.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../../string/string.h"
#include "../assignable/identifier/identifier.h"
#include "../parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void free_destructure_key_item(void *ptr) {
  DestructureKeyItem *item = ptr;

  if (item->key) {
    free_parsed(item->key);
    free(item->key);
  }

  free_destructure(item->destructure);
}

static Destructure *parse_destructure_key(char *file, DArray *tokens,
                                          size_t *index, ArErr *err) {
  // Consume '{'
  (*index)++;

  skip_newlines_and_indents(tokens, index);

  DArray items;
  darray_init(&items, sizeof(DestructureKeyItem));

  Destructure *rest = NULL;

  while (true) {
    ArErr finish_err = error_if_finished(file, tokens, index);
    if (is_error(&finish_err)) {
      darray_free(&items, free_destructure_key_item);
      *err = finish_err;
      return NULL;
    }

    Token *token = darray_get(tokens, *index);

    // {}
    if (token->type == TOKEN_RBRACE) {
      (*index)++;
      break;
    }

    // *rest
    if (token->type == TOKEN_STAR) {
      (*index)++;

      skip_newlines_and_indents(tokens, index);

      rest = parse_destructure(file, tokens, index, err);

      if (is_error(err)) {
        darray_free(&items, free_destructure_key_item);
        return NULL;
      }

      if (!rest) {
        darray_free(&items, free_destructure_key_item);

        *err = path_specific_create_err(
            token->line, token->column, token->length, file, SyntaxError,
            "expected destructuring pattern after '*'");

        return NULL;
      }

      skip_newlines_and_indents(tokens, index);

      finish_err = error_if_finished(file, tokens, index);
      if (is_error(&finish_err)) {
        free_destructure(rest);
        darray_free(&items, free_destructure_key_item);
        *err = finish_err;
        return NULL;
      }

      token = darray_get(tokens, *index);

      if (token->type != TOKEN_RBRACE) {
        free_destructure(rest);
        darray_free(&items, free_destructure_key_item);

        *err = path_specific_create_err(
            token->line, token->column, token->length, file, SyntaxError,
            "rest must be the last item in destructuring");

        return NULL;
      }

      (*index)++;
      break;
    }

    /*
     * Parse the key normally.
     *
     * {name}
     * {(name)}
     * {foo.bar}
     * {42}
     *
     * are all parsed by the normal parser.
     */
    ParsedValueReturn parsed_key = parse_token(file, tokens, index, true);

    if (is_error(&parsed_key.err)) {
      darray_free(&items, free_destructure_key_item);
      *err = parsed_key.err;
      return NULL;
    }

    if (!parsed_key.value) {
      darray_free(&items, free_destructure_key_item);

      *err = path_specific_create_err(token->line, token->column, token->length,
                                      file, SyntaxError, "expected key");

      return NULL;
    }

    ParsedValue *key = parsed_key.value;

    skip_newlines_and_indents(tokens, index);

    finish_err = error_if_finished(file, tokens, index);
    if (is_error(&finish_err)) {
      free_parsed(key);
      free(key);
      darray_free(&items, free_destructure_key_item);
      *err = finish_err;
      return NULL;
    }

    token = darray_get(tokens, *index);

    Destructure *destructure = NULL;

    /*
     * Shorthand:
     *
     *     {name}
     *
     * is equivalent to:
     *
     *     {name: name}
     *
     * but only for a bare identifier.
     */
    if (token->type != TOKEN_COLON) {
      if (key->type != AST_IDENTIFIER) {
        free_parsed(key);
        free(key);
        darray_free(&items, free_destructure_key_item);

        *err = path_specific_create_err(token->line, token->column,
                                        token->length, file, SyntaxError,
                                        "expected ':' after destructuring key");

        return NULL;
      }

      ParsedIdentifier *identifier = key->data;

      destructure = checked_malloc(sizeof(Destructure));

      destructure->type = DESTRUCTURE_IDENTIFIER;
      destructure->identifier.name = cloneString(identifier->name);
    } else {
      // Explicit key: destructure
      (*index)++;

      skip_newlines_and_indents(tokens, index);

      finish_err = error_if_finished(file, tokens, index);
      if (is_error(&finish_err)) {
        free_parsed(key);
        free(key);
        darray_free(&items, free_destructure_key_item);
        *err = finish_err;
        return NULL;
      }

      destructure = parse_destructure(file, tokens, index, err);

      if (is_error(err)) {
        free_parsed(key);
        free(key);
        darray_free(&items, free_destructure_key_item);
        return NULL;
      }

      if (!destructure) {
        free_parsed(key);
        free(key);
        darray_free(&items, free_destructure_key_item);

        *err = path_specific_create_err(
            token->line, token->column, token->length, file, SyntaxError,
            "expected destructuring pattern after ':'");

        return NULL;
      }
    }

    DestructureKeyItem item = {.key = key, .destructure = destructure};

    darray_push(&items, &item);

    skip_newlines_and_indents(tokens, index);

    finish_err = error_if_finished(file, tokens, index);
    if (is_error(&finish_err)) {
      darray_free(&items, free_destructure_key_item);
      *err = finish_err;
      return NULL;
    }

    token = darray_get(tokens, *index);

    if (token->type == TOKEN_RBRACE) {
      (*index)++;
      break;
    }

    if (token->type != TOKEN_COMMA) {
      darray_free(&items, free_destructure_key_item);

      *err = path_specific_create_err(token->line, token->column, token->length,
                                      file, SyntaxError, "expected ',' or '}'");

      return NULL;
    }

    (*index)++;

    skip_newlines_and_indents(tokens, index);
  }

  size_t length = items.size;

  Destructure *result = checked_malloc(sizeof(Destructure));

  result->type = DESTRUCTURE_KEY;
  result->key.length = length;
  result->key.rest = rest;

  if (length > 0) {
    result->key.items = checked_malloc(sizeof(DestructureKeyItem) * length);

    for (size_t i = 0; i < length; i++) {
      result->key.items[i] = *(DestructureKeyItem *)darray_get(&items, i);
    }
  } else {
    result->key.items = NULL;
  }

  /*
   * Ownership of the items has moved into result.
   */
  darray_free(&items, NULL);

  *err = no_err;
  return result;
}

/*
 * `items` in parse_destructure_index stores Destructure* VALUES (not
 * Destructure structs) inline in the array. darray_free() invokes its
 * callback with the ADDRESS of each slot, so the callback must dereference
 * that address to get the actual Destructure* before freeing it. Passing
 * free_destructure directly (cast to match the callback signature) is a
 * bug: it treats the slot's address as if it were the Destructure* itself,
 * corrupting/freeing the array's own backing buffer instead of the
 * pointed-to objects, and leaking the real Destructure objects. This
 * wrapper does the necessary dereference.
 */
static void free_destructure_item(void *ptr) {
  Destructure *item = *(Destructure **)ptr;
  if (item) free_destructure(item);
}

static Destructure *parse_destructure_index(char *file, DArray *tokens,
                                            size_t *index, ArErr *err) {
  // Consume '['
  (*index)++;

  skip_newlines_and_indents(tokens, index);

  DArray items;
  darray_init(&items, sizeof(Destructure *));

  Destructure *rest = NULL;

  while (true) {
    ArErr finish_err = error_if_finished(file, tokens, index);
    if (is_error(&finish_err)) {
      darray_free(&items, free_destructure_item);
      *err = finish_err;
      return NULL;
    }

    Token *token = darray_get(tokens, *index);

    // []
    if (token->type == TOKEN_RBRACKET) {
      (*index)++;
      break;
    } else if (token->type == TOKEN_COMMA) {

      Destructure* item = NULL;

      darray_push(&items, &item);

      (*index)++;
      skip_newlines_and_indents(tokens, index);
      continue;
    }

    // *rest
    if (token->type == TOKEN_STAR) {
      (*index)++;

      skip_newlines_and_indents(tokens, index);

      rest = parse_destructure(file, tokens, index, err);

      if (is_error(err)) {
        darray_free(&items, free_destructure_item);
        return NULL;
      }

      if (!rest) {
        darray_free(&items, free_destructure_item);

        *err = path_specific_create_err(
            token->line, token->column, token->length, file, SyntaxError,
            "expected destructuring pattern after '*'");

        return NULL;
      }

      skip_newlines_and_indents(tokens, index);

      finish_err = error_if_finished(file, tokens, index);
      if (is_error(&finish_err)) {
        free_destructure(rest);
        darray_free(&items, free_destructure_item);

        *err = finish_err;
        return NULL;
      }

      token = darray_get(tokens, *index);

      if (token->type != TOKEN_RBRACKET) {
        free_destructure(rest);
        darray_free(&items, free_destructure_item);

        *err = path_specific_create_err(
            token->line, token->column, token->length, file, SyntaxError,
            "rest must be the last item in destructuring");

        return NULL;
      }

      (*index)++;
      break;
    }

    Destructure *item = parse_destructure(file, tokens, index, err);

    if (is_error(err)) {
      darray_free(&items, free_destructure_item);
      return NULL;
    }

    if (!item) {
      darray_free(&items, free_destructure_item);

      *err = path_specific_create_err(token->line, token->column, token->length,
                                      file, SyntaxError,
                                      "expected destructuring pattern");

      return NULL;
    }

    darray_push(&items, &item);

    skip_newlines_and_indents(tokens, index);

    finish_err = error_if_finished(file, tokens, index);
    if (is_error(&finish_err)) {
      darray_free(&items, free_destructure_item);

      *err = finish_err;
      return NULL;
    }

    token = darray_get(tokens, *index);

    // []  (closing after an item, e.g. the trailing ']' in `[a,b,c]`)
    if (token->type == TOKEN_RBRACKET) {
      (*index)++;
      break;
    }

    if (token->type != TOKEN_COMMA) {
      darray_free(&items, free_destructure_item);

      *err = path_specific_create_err(token->line, token->column, token->length,
                                      file, SyntaxError, "expected ',' or ']'");

      return NULL;
    }

    (*index)++;
    skip_newlines_and_indents(tokens, index);
  }

  size_t length = items.size;

  Destructure *result = checked_malloc(sizeof(Destructure));

  result->type = DESTRUCTURE_INDEX;
  result->index.length = length;
  result->index.rest = rest;

  if (length > 0) {
    result->index.items = checked_malloc(sizeof(Destructure *) * length);

    for (size_t i = 0; i < length; i++) {
      result->index.items[i] = *(Destructure **)darray_get(&items, i);
    }
  } else {
    result->index.items = NULL;
  }

  // Don't free the Destructure objects themselves!
  // Ownership has moved to result->index.items.
  darray_free(&items, NULL);

  *err = no_err;
  return result;
}

Destructure *parse_destructure(char *file, DArray *tokens, size_t *index,
                               ArErr *err) {
  skip_newlines_and_indents(tokens, index);

  ArErr finish_err = error_if_finished(file, tokens, index);

  if (is_error(&finish_err)) {
    *err = finish_err;
    return NULL;
  }

  Token *token = darray_get(tokens, *index);

  switch (token->type) {
  case TOKEN_IDENTIFIER: {
    Destructure *result = checked_malloc(sizeof(Destructure));

    result->type = DESTRUCTURE_IDENTIFIER;
    result->identifier.name = cloneString(token->value);
    result->identifier.column = token->column;
    result->identifier.line = token->line;

    (*index)++;

    *err = no_err;
    return result;
  }

  case TOKEN_LBRACKET:
    return parse_destructure_index(file, tokens, index, err);

  case TOKEN_LBRACE:
    return parse_destructure_key(file, tokens, index, err);

  default:
    *err = path_specific_create_err(
        token->line, token->column, token->length, file, SyntaxError,
        "expected identifier or destructuring pattern");
    return NULL;
  }
}

void free_destructure(Destructure *destructure) {
  if (!destructure) {
    return;
  }

  switch (destructure->type) {
  case DESTRUCTURE_IDENTIFIER:
    free(destructure->identifier.name);
    break;

  case DESTRUCTURE_INDEX:
    for (size_t i = 0; i < destructure->index.length; i++) {
      free_destructure(destructure->index.items[i]);
    }

    free(destructure->index.items);

    free_destructure(destructure->index.rest);
    break;

  case DESTRUCTURE_KEY:
    for (size_t i = 0; i < destructure->key.length; i++) {
      free_parsed(destructure->key.items[i].key);
      free(destructure->key.items[i].key);

      free_destructure(destructure->key.items[i].destructure);
    }

    free(destructure->key.items);

    free_destructure(destructure->key.rest);
    break;
  }

  free(destructure);
}