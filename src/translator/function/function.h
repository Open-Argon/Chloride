#ifndef BYTECODE_FUNCTION_H
#define BYTECODE_FUNCTION_H
#include "../../parser/function/function.h"
#include "../translator.h"

size_t translate_parsed_function(Translated *translated,
                                 ParsedFunction *parsedFunction);

#endif