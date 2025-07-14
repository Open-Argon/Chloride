/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_DECLARATION_H
#define BYTECODE_DECLARATION_H
#include "../translator.h"

size_t translate_parsed_declaration(Translated *translated,
                                    DArray delcarations, ArErr *err);

#endif