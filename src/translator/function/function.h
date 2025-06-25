#ifndef BYTECODE_FUNCTION_H
#define BYTECODE_FUNCTION_H
#include "../translator.h"
#include "../../parser/function/function.h"

size_t translate_parsed_function(Translated *translated,
                                ParsedFunction *parsedFunction);

#endif