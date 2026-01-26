/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_CLASS_H
#define TRANSLATE_CLASS_H
#include "../../parser/class/class.h"


size_t translate_parsed_class(Translated *translated,
                               ParsedClass *parsedClass, ArErr * err);

#endif // TRANSLATE_CLASS_H