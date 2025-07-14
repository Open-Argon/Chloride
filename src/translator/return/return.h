/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_RETURN_H
#define TRANSLATE_RETURN_H
#include "../translator.h"
#include "../../parser/return/return.h"


size_t translate_parsed_return(Translated *translated,
                               ParsedReturn *parsedReturn, ArErr * err);

#endif // TRANSLATE_RETURN_H