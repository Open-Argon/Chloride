/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef native_api_H
#define native_api_H
#include "../../arobject.h"

struct number {
  int64_t n;
  int64_t d;
};

extern ArgonNativeAPI native_api;

#endif // native_api_H