/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_FOR_H
#define TRANSLATE_FOR_H
#include "../../parser/for/for.h"

size_t translate_parsed_for(Translated *translated, ParsedFor *parsedFor,
                              ArErr *err);

#endif // TRANSLATE_FOR_H