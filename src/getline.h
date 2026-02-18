/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GETLINE_H
#define GETLINE_H
#if defined(_WIN32) || defined(_WIN64)
FILE *fmemopen(void *buf, size_t size, const char *mode);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif
#endif // GETLINE_H