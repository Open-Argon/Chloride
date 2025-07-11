#ifndef HASH_DATA_H
#define HASH_DATA_H
#include <stdlib.h>
#include <stdint.h>

extern uint8_t siphash_key[16];
extern uint8_t empty_siphash_key[16];

void generate_siphash_key(uint8_t siphash_key[16]);
uint64_t siphash64_bytes(const void *data, size_t len,uint8_t siphash_key[16]);
#endif //HASH_DATA_H