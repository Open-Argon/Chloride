#include "translator.h"
#include "../dynamic_array/darray.h"
#include <stdint.h>
#include <stdlib.h>

Translated *init_translator() {
  Translated *translated = malloc(sizeof(Translated));
  if (!translated)
    return NULL;

  darray_init(&translated->bytecode, sizeof(uint8_t));
  return translated;
}