/*
 * SPDX-FileCopyrightText: 2025 William Bell
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

char *swap_quotes(char *input, char quote);

char *unquote(char *str, size_t *decoded_len);

ParsedValueReturn parse_string(Token* token, bool to_unquote);

void free_parsed_string(void *ptr);

#endif // STRING_UTILS_H