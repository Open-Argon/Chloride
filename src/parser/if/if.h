/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef iF_H
#define iF_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

typedef struct {
  ParsedValue * condition; // NULL for 'else'
  ParsedValue *content;
} ParsedConditional;

ParsedValueReturn parse_if(char *file, DArray *tokens, size_t *index);

void free_parsed_if(void *ptr);

#endif // iF_H