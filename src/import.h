/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IMPORT_H
#define IMPORT_H
#include "arobject.h"
#include <linux/limits.h>

extern char CWD[PATH_MAX];
extern char EXC[PATH_MAX];
extern char EXC_DIR[PATH_MAX];
extern ArgonObject *CWD_ARGON;
extern ArgonObject *EXC_ARGON;

extern const char version_string[];

int get_executable_path(char*path, size_t size);

extern struct hashmap_GC *importing_hash_table;
extern struct hashmap_GC *imported_hash_table;

Stack *ar_import(char *current_directory, char *path_relative, ArErr *err,
                 bool is_main);

#endif // IMPORT_H