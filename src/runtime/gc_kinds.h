/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// The interpreter's managed types, as the garbage collector sees them.
// See src/gc/README.md.

#ifndef ARGON_GC_KINDS_H
#define ARGON_GC_KINDS_H

#include <stdint.h>

extern uint8_t ARGON_OBJECT_GC_KIND; // ArgonObject
extern uint8_t STACK_GC_KIND;        // Stack (a scope)

// Call once, right after ar_memory_init and before allocating any object.
void register_gc_kinds(void);

#endif // ARGON_GC_KINDS_H
