/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "../../lexer/token.h"
#include "../parser.h"

// Declare functions related to string processing in parser

typedef struct {
  size_t length;
  char *string;
} ParsedString;

typedef struct {
  DArray values;
  ParsedValue *templater;
} ParsedTemplate;

typedef struct {
  bool is_string;
  union {
    ParsedString string;
    ParsedValue *value;
  } value;
} TemplateValue;

// Decodes the escapes in a string literal. On failure returns NULL and, if
// error is not NULL, sets it to a description of the problem.
char *unquote(char *input, size_t *out_len, char quote_char, bool is_quoted,
              const char **error);

ParsedValueReturn parse_string(char *file, Token *token, bool to_unquote);

void free_parsed_string(void *ptr);

ParsedValueReturn parse_template(char *file, DArray *tokens, size_t *index,
                                 ParsedValue *templater);

void free_parsed_template(void *ptr);

#endif // STRING_UTILS_H