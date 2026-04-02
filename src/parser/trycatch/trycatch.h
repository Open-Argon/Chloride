/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRYCATCH_H
#define TRYCATCH_H
#include "../parser.h"

typedef struct {
} ParsedTry;

ParsedValueReturn parse_try(char *file, DArray *tokens, size_t *index);

void free_parsed_try(void *ptr);

#endif // TRYCATCH_H