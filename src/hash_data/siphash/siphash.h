/*
 * Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
 * Copyright (c) 2012-2022 Jean-Philippe Aumasson
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <string.h>

int siphash(const void *in, const size_t inlen, const void *k, uint8_t *out,
            const size_t outlen);