/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../../internals/hashmap/hashmap.h"
#include "../object.h"
#include <string.h>
#include "type.h"

ArgonObject *ARGON_TYPE = NULL;

void init_type() {
    ARGON_TYPE = init_argon_class("type");
    ARGON_TYPE->baseObject = BASE_CLASS;
}