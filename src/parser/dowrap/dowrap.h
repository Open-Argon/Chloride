/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DOWRAP_H
#define DOWRAP_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

// Function declaration for parsing an identifier
ParsedValueReturn parse_dowrap(char *file, DArray *tokens, size_t *index);

void free_dowrap(void *ptr);

#endif // DOWRAP_H