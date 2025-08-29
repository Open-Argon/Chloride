/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_ASSIGNMENT_H
#define BYTECODE_ASSIGNMENT_H
#include "../translator.h"
#include "../../parser/assignable/assign/assign.h"

size_t translate_parsed_assignment(Translated *translated,
                                   ParsedAssign *assignment, ArErr *err);

#endif