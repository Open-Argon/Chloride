/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef native_api_H
#define native_api_H
#include "../../arobject.h"

extern ArgonNativeAPI native_api;
bool is_error(ArErr *err);

#endif // native_api_H