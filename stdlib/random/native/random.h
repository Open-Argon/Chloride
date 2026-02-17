// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t random_os_seed(void);

/* Deterministic non-crypto PRNG (PCG32) */

typedef struct {
    uint64_t state;
    uint64_t inc;
} Random;

/* Initialize with a deterministic seed */
void random_seed(Random *r, uint64_t seed);

/* Next random 32-bit unsigned integer */
uint32_t random_next_u32(Random *r);

/* Next random double in [0.0, 1.0), 53-bit precision */
double random_next_double(Random *r);

#ifdef __cplusplus
}
#endif

#endif /* RANDOM_H */
