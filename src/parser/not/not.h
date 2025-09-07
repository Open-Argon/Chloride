/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NOT_H
#define NOT_H
#include "../parser.h"

typedef struct {
  bool invert;
  ParsedValue*value;
} ParsedToBool;

ParsedValueReturn parse_not(char *file, DArray *tokens, size_t *index);

void free_not(void *ptr);

#endif // NOT_H