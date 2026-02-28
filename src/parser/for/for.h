/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PARSE_FOR_H
#define PARSE_FOR_H
#include "../parser.h"

typedef struct {
  char* key;
  ParsedValue * iterator;
  ParsedValue *content;
} ParsedFor;

ParsedValueReturn parse_for(char *file, DArray *tokens, size_t *index);

void free_parsed_for(void *ptr);

#endif // PARSE_FOR_H