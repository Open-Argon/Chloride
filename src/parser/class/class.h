/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CLASS_H
#define CLASS_H
#include "../parser.h"

typedef struct {
  char *name;
  ParsedValue *parent;
  ParsedValue *body;
  size_t line;
  size_t column;
} ParsedClass;

ParsedValueReturn parse_class(char *file, DArray *tokens, size_t *index);

void free_class(void *ptr);

#endif // CLASS_H