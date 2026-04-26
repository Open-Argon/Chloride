/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TRANSLATE_DELETE_H
#define TRANSLATE_DELETE_H
#include "../../parser/delete/delete.h"


size_t translate_parsed_delete(Translated *translated,
                               ParsedDelete *parsedDelete, ArErr * err);

#endif // TRANSLATE_DELETE_H