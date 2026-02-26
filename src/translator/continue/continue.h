/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_CONTINUE_H
#define TRANSLATE_CONTINUE_H
#include "../../parser/continue/continue.h"


size_t translate_parsed_continue(Translated *translated,
                               ParsedContinue *parsedContinue, ArErr * err);

#endif // TRANSLATE_CONTINUE_H