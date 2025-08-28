/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PARSE_WHILE_H
#define PARSE_WHILE_H
#include "../parser.h"

typedef struct {
  ParsedValue * condition;
  ParsedValue *content;
} ParsedWhile;

ParsedValueReturn parse_while(char *file, DArray *tokens, size_t *index);

void free_parsed_while(void *ptr);

#endif // PARSE_WHILE_H