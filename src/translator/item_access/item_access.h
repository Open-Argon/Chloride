/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef translator_item_access_H
#define translator_item_access_H
#include "../../parser/assignable/item/item.h"
#include "../translator.h"

size_t translate_item_access(Translated *translated, ParsedItemAccess *access,
                        ArErr *err);

#endif // translator_item_access_H