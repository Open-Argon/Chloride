#ifndef BYTECODE_DECLARATION_H
#define BYTECODE_DECLARATION_H
#include "../translator.h"

size_t translate_parsed_declaration(Translated *translated,
                                  DArray delcarations);

#endif