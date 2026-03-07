/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LEXER_H
#define LEXER_H

#include "../arobject.h"
#include "../dynamic_array/darray.h"
#include <stdio.h>
#define TEMPLATE_STACK_MAX 64

typedef struct {
  char *path;
  char *content;
  size_t current_line;
  size_t current_column;
  int template_paren_stack[TEMPLATE_STACK_MAX];
  int template_stack_top;
  DArray *tokens;
  // add more fields as needed
} LexerState;

ArErr lexer(LexerState state);

#endif // LEXER_H