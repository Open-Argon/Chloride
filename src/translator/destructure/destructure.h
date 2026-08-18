#ifndef TRANSLATE_DESTRUCTURE_H
#define TRANSLATE_DESTRUCTURE_H

#include "../../parser/destructure/destructure.h"
#include "../translator.h"
#include <stdint.h>

size_t translate_destructure(Translated *translated, Destructure *destructure,
                             uint8_t value_register, ArErr *err, OperationType opcode);

#endif // TRANSLATE_DESTRUCTURE_H