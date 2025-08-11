/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NUMBER_H
#define NUMBER_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

// Function declaration for parsing an identifier
ParsedValueReturn parse_number(Token *token, char*path);

#endif // NUMBER_H