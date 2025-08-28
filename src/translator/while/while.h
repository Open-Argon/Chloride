/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_WHILE_H
#define TRANSLATE_WHILE_H
#include "../translator.h"
#include "../../parser/while/while.h"

size_t translate_parsed_while(Translated *translated, ParsedWhile *parsedWhile,
                              ArErr *err);

#endif // TRANSLATE_WHILE_H