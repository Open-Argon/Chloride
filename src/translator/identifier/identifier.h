#ifndef BYTECODE_IDENTIFIER_H
#define BYTECODE_IDENTIFIER_H
#include "../translator.h"
#include "../../parser/assignable/identifier/identifier.h"

size_t translate_parsed_identifier(Translated *translated, char* identifier);

#endif