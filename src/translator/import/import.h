/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BYTECODE_IMPORT_H
#define BYTECODE_IMPORT_H
#include "../../parser/import/import.h"

size_t translate_parsed_import(Translated *translated,
                               ParsedImport *parsedImport, ArErr *err);

#endif // BYTECODE_IMPORT_H