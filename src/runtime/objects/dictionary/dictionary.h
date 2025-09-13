/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DICTIONARY_H
#define DICTIONARY_H
#include "../object.h"

extern ArgonObject *ARGON_DICTIONARY_TYPE;

void create_ARGON_DICTIONARY_TYPE();

ArgonObject* create_dictionary(struct hashmap_GC*hashmap);

#endif // DICTIONARY_H