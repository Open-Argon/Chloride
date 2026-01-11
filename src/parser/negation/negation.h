/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NEGATION_H
#define NEGATION_H
#include "../parser.h"

ParsedValueReturn parse_negation(char *file, DArray *tokens, size_t *index);

void free_negation(void *ptr);

#endif // NEGATION_H