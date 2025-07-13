/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "siphash/siphash.h"
#include "hash_data.h"

uint8_t siphash_key[16];
const uint8_t siphash_key_fixed_for_translator[16] = {218, 19, 245, 136, 128, 160, 122, 81, 249, 147, 111, 230, 174, 145, 125 ,218};
uint8_t empty_siphash_key[16];

void generate_siphash_key(uint8_t hash_key[16]) {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0 || read(fd, hash_key, 16) != 16) {
    // Fallback or abort
  }
  close(fd);
}

uint64_t siphash64_bytes(const void *data, size_t len,const uint8_t hash_key[16]) {
  uint8_t out[8];
  if (siphash(data, len, hash_key, out, sizeof(out)) != 0)
    return 0;

  uint64_t hash = 0;
  for (int i = 0; i < 8; ++i)
    hash |= ((uint64_t)out[i]) << (8 * i);

  return hash;
}