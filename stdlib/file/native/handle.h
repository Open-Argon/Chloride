// SPDX-FileCopyrightText: 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    FILE_NORMAL,
    FILE_STD,
    FILE_NULL
} FileType;


typedef struct {
    FILE *fp;
    bool is_open;
    FileType type;
} FileHandle;

#endif