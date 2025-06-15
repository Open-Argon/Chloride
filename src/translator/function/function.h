#ifndef BYTECODE_FUNCTION_H
#define BYTECODE_FUNCTION_H
#include "../translator.h"

size_t translate_parsed_function(Translated *translated,
                                  ParsedValue *parsedValue);

#endif