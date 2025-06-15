#ifndef BYTECODE_STRING_H
#define BYTECODE_STRING_H
#include "../translator.h"
#include "../../parser/string/string.h"

size_t translate_parsed_string(Translated *translated, ParsedString parsedString, size_t to_register);

#endif