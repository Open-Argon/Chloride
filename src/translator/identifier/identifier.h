/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_IDENTIFIER_H
#define BYTECODE_IDENTIFIER_H
#include "../translator.h"
#include "../../parser/assignable/identifier/identifier.h"

size_t translate_parsed_identifier(Translated *translated, ParsedIdentifier *parsedIdentifier);

#endif