/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IMPORT_H
#define IMPORT_H
#include "arobject.h"

extern char*CWD;
extern char*EXC;
extern ArgonObject*CWD_ARGON;
extern ArgonObject*EXC_ARGON;

extern const char version_string[];

char* get_executable_path();

extern struct hashmap *importing_hash_table;
extern struct hashmap_GC *imported_hash_table;

Stack *ar_import(char *current_directory, char *path_relative, ArErr *err, bool main);

#endif // IMPORT_H