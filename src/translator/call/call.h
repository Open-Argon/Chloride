/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_CALL_H
#define BYTECODE_CALL_H
#include "../../parser/assignable/call/call.h"
#include "../translator.h"
#include <stddef.h>

size_t translate_parsed_call(Translated *translated, ParsedCall* call, ArErr *err);

#endif