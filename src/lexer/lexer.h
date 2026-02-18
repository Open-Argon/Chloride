/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LEXER_H
#define LEXER_H

#include "../dynamic_array/darray.h"
#include <stdio.h>
#include "../arobject.h"

typedef struct {
  char *path;
  char *content;
  size_t current_line;
  size_t current_column;
  DArray *tokens;
  // add more fields as needed
} LexerState;

ArErr lexer(LexerState state);

#endif // LEXER_H