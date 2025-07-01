/*
 * This file is dedicated to the public domain under CC0 1.0 Universal.
 * See the LICENSE-CC0 file in this directory for details.
 *
 * SipHash reference C implementation
 *
 * Copyright (c) 2012-2022 Jean-Philippe Aumasson
 *   <jeanphilippe.aumasson@gmail.com>
 * Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <string.h>

int siphash(const void *in, const size_t inlen, const void *k, uint8_t *out,
            const size_t outlen);