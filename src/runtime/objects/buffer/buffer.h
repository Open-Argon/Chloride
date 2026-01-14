/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include "../object.h"

ArgonObject *create_ARGON_BUFFER_object(size_t size);
void resize_ARGON_BUFFER_object(ArgonObject *obj, ArErr *err, size_t new_size);
struct buffer ARGON_BUFFER_to_buffer_struct(ArgonObject *obj,
                                                   ArErr *err);

#endif // BUFFER_H