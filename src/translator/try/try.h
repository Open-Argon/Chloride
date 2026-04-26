/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_TRY_H
#define TRANSLATE_TRY_H
#include "../../parser/trycatch/trycatch.h"

size_t translate_parsed_try(Translated *translated, ParsedTry *parsedTry,
                              ArErr *err);

#endif // TRANSLATE_TRY_H