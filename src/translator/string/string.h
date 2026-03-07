/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_STRING_H
#define BYTECODE_STRING_H
#include "../../parser/string/string.h"

size_t translate_parsed_string(Translated *translated, ParsedString parsedString);

size_t translate_parsed_template(Translated *translated, ParsedTemplate *parsedTemplate,
                              ArErr *err);

#endif