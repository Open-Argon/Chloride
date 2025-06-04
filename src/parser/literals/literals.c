#include "literals.h"
#include "../parser.h"
#include <stdbool.h>
#include "../../memory.h"

static bool true_value = true;
static bool false_value = false;

ParsedValue * parse_true(){
    ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
    parsedValue->type = AST_BOOLEAN;
    parsedValue->data = &true_value;
    return parsedValue;
};

ParsedValue * parse_false(){
    ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
    parsedValue->type = AST_BOOLEAN;
    parsedValue->data = &false_value;
    return parsedValue;
};


ParsedValue * parse_null(){
    ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
    parsedValue->type = AST_NULL;
    parsedValue->data = NULL;
    return parsedValue;
};