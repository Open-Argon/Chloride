#ifndef BYTECODE_DECLARATION_H
#define BYTECODE_DECLARATION_H
#include "../translator.h"

size_t translate_parsed_string(Translated *translated, ParsedValue *parsedValue);

size_t translate_parsed_declaration(Translated *translated,
                                  ParsedValue *parsedValue);

#endif