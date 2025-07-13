/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CLONESTRING_H
#define CLONESTRING_H

extern const char *WHITE_SPACE;

char *cloneString(char *str);

void stripString(char *str, const char *chars);

#endif // CLONESTRING_H
