/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_NUMBER_H
#define RUNTIME_NUMBER_H
#include "../object.h"

extern ArgonObject *ARGON_NUMBER_TYPE;

void create_ARGON_NUMBER_TYPE();

ArgonObject *new_number_object(char *data);

#endif // RUNTIME_NUMBER_H