#ifndef BYTECODE_NUMBER_H
#define BYTECODE_NUMBER_H
#include "../translator.h"

size_t translate_parsed_number(Translated *translated, char *number_str, size_t to_register);

#endif