/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_BREAK_H
#define TRANSLATE_BREAK_H
#include "../../parser/continue/continue.h"


size_t translate_parsed_break(Translated *translated,
                               ParsedContinueOrBreak *parsedBreak, ArErr * err);

#endif // TRANSLATE_BREAK_H