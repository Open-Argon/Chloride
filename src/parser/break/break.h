/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BREAK_H
#define BREAK_H

#include "../parser.h"

ParsedValueReturn parse_break(DArray *tokens, size_t *index);

void free_parsed_break(void *ptr);

#endif // BREAK_H