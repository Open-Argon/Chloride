/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_OPERATION_H
#define BYTECODE_OPERATION_H
#include "../translator.h"
#include "../../parser/operations/operations.h"

size_t translate_operation(Translated *translated, ParsedOperation *operation,
                        ArErr *err);

#endif