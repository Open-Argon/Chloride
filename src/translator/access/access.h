/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef translator_access_H
#define translator_access_H
#include "../../parser/assignable/access/access.h"
#include "../translator.h"

size_t translate_access(Translated *translated, ParsedAccess *access,
                        ArErr *err);

#endif // translator_access_H