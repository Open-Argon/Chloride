/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "literals.h"
#include "../parser.h"
#include <stdbool.h>
#include "../../memory.h"

ParsedValue * parse_true(){
    ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
    parsedValue->type = AST_BOOLEAN;
    parsedValue->data = (void*)true;
    return parsedValue;
};

ParsedValue * parse_false(){
    ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
    parsedValue->type = AST_BOOLEAN;
    parsedValue->data = (void*)false;
    return parsedValue;
};


ParsedValue * parse_null(){
    ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
    parsedValue->type = AST_NULL;
    parsedValue->data = NULL;
    return parsedValue;
};