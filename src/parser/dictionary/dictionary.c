#include "dictionary.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include "../string/string.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValueReturn parse_dictionary(char *file, DArray *tokens, size_t *index) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_DICTIONARY;
  DArray *dictionary = checked_malloc(sizeof(DArray));
  parsedValue->data = dictionary;
  darray_init(dictionary, sizeof(ParsedValue));
  (*index)++;
  skip_newlines_and_indents(tokens, index);
  error_if_finished(file, tokens, index);
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_RBRACE) {
    while (true) {
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      size_t keyIndex = *index;
      Token *keyToken = darray_get(tokens, *index);
      ParsedValueReturn key;
      if (keyToken->type == TOKEN_IDENTIFIER) {
        (*index)++;
        key = parse_string(keyToken, false);
      } else {
        key = parse_token(file, tokens, index, true);
      }
      if (key.err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return key;
      } else if (!key.value) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected key"),
                                   NULL};
      }
      skip_newlines_and_indents(tokens, index);
      error_if_finished(file, tokens, index);
      token = darray_get(tokens, *index);
      ParsedValueReturn value;
      bool tobreak = false;
      switch (token->type) {
      case TOKEN_COLON:
        (*index)++;
        skip_newlines_and_indents(tokens, index);
        error_if_finished(file, tokens, index);
        value = parse_token(file, tokens, index, true);
        if (value.err.exists) {
          free_parsed(parsedValue);
          free(parsedValue);
          free_parsed(key.value);
          free(key.value);
          return value;
        } else if (!value.value) {
          free_parsed(parsedValue);
          free(parsedValue);
          free_parsed(key.value);
          free(key.value);
          return (ParsedValueReturn){
              create_err(token->line, token->column, token->length, file,
                         "Syntax Error", "expected value"),
              NULL};
        }
        skip_newlines_and_indents(tokens, index);
        error_if_finished(file, tokens, index);
        token = darray_get(tokens, *index);
        if (token->type == TOKEN_RBRACE) {
          tobreak = true;
        } else if (token->type != TOKEN_COMMA) {
          fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
                  token->column);
          exit(EXIT_FAILURE);
        }
        break;
      case TOKEN_RBRACE:
        tobreak = true;
        /* fall through */
      case TOKEN_COMMA:
        value = parse_token(file, tokens, &keyIndex, true);
        break;
      default:
        fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
                token->column);
        exit(EXIT_FAILURE);
      }
      ParsedDictionaryEntry entry = {key.value, value.value};
      darray_push(dictionary, &entry);
      if (tobreak) {
        break;
      }
      (*index)++;
      error_if_finished(file, tokens, index);
    }
  }
  (*index)++;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_dictionary_entry(void *ptr) {
  ParsedDictionaryEntry *entry = ptr;
  free_parsed(entry->key);
  free(entry->key);
  free_parsed(entry->value);
  free(entry->value);
}

void free_parsed_dictionary(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed_dictionary = parsedValue->data;
  darray_free(parsed_dictionary, free_dictionary_entry);
  free(parsed_dictionary);
}