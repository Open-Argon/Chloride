/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IMPORT_H
#define IMPORT_H
#include "err.h"

extern char*CWD;

Stack *ar_import(char *current_directory, char *path_relative, ArErr *err);

#endif // IMPORT_H