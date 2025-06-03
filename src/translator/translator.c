#include "translator.h"
#include "../dynamic_array/darray.h"
#include <stdint.h>
#include <stdlib.h>
#include "../memory.h"

Translated *init_translator() {
  Translated *translated = checked_malloc(sizeof(Translated));
  if (!translated)
    return NULL;

  darray_init(&translated->bytecode, sizeof(uint8_t));
  return translated;
}