/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "for.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../../memory.h"
#include "../parser.h"
#include <stddef.h>

ParsedValueReturn parse_for(char *file, DArray *tokens, size_t *index) {
    // Consume 'for'
    (*index)++;

    // Parse ( destructure in iterator )
    Token *token = darray_get(tokens, *index);

    if (token->type != TOKEN_LPAREN) {
        return (ParsedValueReturn){
            path_specific_create_err(
                token->line,
                token->column,
                token->length,
                file,
                SyntaxError,
                "expected '(' after for"
            ),
            NULL
        };
    }

    (*index)++;

    skip_newlines_and_indents(tokens, index);

    ArErr err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
        return (ParsedValueReturn){err, NULL};
    }

    /*
     * Parse the destructuring pattern.
     *
     * This can now be:
     *
     *     x
     *     [x, y]
     *     [x, [y, z]]
     *     {name: x, age: y}
     *     etc.
     */
    ArErr destructure_err;
    Destructure *value = parse_destructure(
        file,
        tokens,
        index,
        &destructure_err
    );

    if (is_error(&destructure_err)) {
        return (ParsedValueReturn){destructure_err, NULL};
    }

    if (!value) {
        token = darray_get(tokens, *index);

        return (ParsedValueReturn){
            path_specific_create_err(
                token->line,
                token->column,
                token->length,
                file,
                SyntaxError,
                "expected destructuring pattern"
            ),
            NULL
        };
    }

    skip_newlines_and_indents(tokens, index);

    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
        free_destructure(value);
        return (ParsedValueReturn){err, NULL};
    }

    token = darray_get(tokens, *index);

    if (token->type != TOKEN_IN) {
        free_destructure(value);

        return (ParsedValueReturn){
            path_specific_create_err(
                token->line,
                token->column,
                token->length,
                file,
                SyntaxError,
                "expected 'in'"
            ),
            NULL
        };
    }

    (*index)++;

    skip_newlines_and_indents(tokens, index);

    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
        free_destructure(value);
        return (ParsedValueReturn){err, NULL};
    }

    ParsedValueReturn iterator =
        parse_token(file, tokens, index, true);

    if (is_error(&iterator.err)) {
        free_destructure(value);
        return iterator;
    }

    if (!iterator.value) {
        free_destructure(value);

        token = darray_get(tokens, *index);

        return (ParsedValueReturn){
            path_specific_create_err(
                token->line,
                token->column,
                token->length,
                file,
                SyntaxError,
                "expected iterator"
            ),
            NULL
        };
    }

    skip_newlines_and_indents(tokens, index);

    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
        free_destructure(value);
        free_parsed(iterator.value);
        free(iterator.value);

        return (ParsedValueReturn){err, NULL};
    }

    token = darray_get(tokens, *index);

    if (token->type != TOKEN_RPAREN) {
        free_destructure(value);
        free_parsed(iterator.value);
        free(iterator.value);

        return (ParsedValueReturn){
            path_specific_create_err(
                token->line,
                token->column,
                token->length,
                file,
                SyntaxError,
                "missing closing ')' in iterator"
            ),
            NULL
        };
    }

    (*index)++;

    skip_newlines_and_indents(tokens, index);

    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
        free_destructure(value);
        free_parsed(iterator.value);
        free(iterator.value);

        return (ParsedValueReturn){err, NULL};
    }

    // Parse the body
    ParsedValueReturn parsed_content =
        parse_token(file, tokens, index, false);

    if (is_error(&parsed_content.err)) {
        free_destructure(value);
        free_parsed(iterator.value);
        free(iterator.value);

        return parsed_content;
    }

    if (!parsed_content.value) {
        free_destructure(value);
        free_parsed(iterator.value);
        free(iterator.value);

        token = darray_get(tokens, *index);

        return (ParsedValueReturn){
            path_specific_create_err(
                token->line,
                token->column,
                token->length,
                file,
                SyntaxError,
                "expected body"
            ),
            NULL
        };
    }

    ParsedValue *parsed_value =
        checked_malloc(sizeof(ParsedValue));

    parsed_value->type = AST_FOR;

    ParsedFor *parsed_for =
        checked_malloc(sizeof(ParsedFor));

    parsed_value->data = parsed_for;

    parsed_for->value = value;
    parsed_for->iterator = iterator.value;
    parsed_for->content = parsed_content.value;

    return (ParsedValueReturn){
        no_err,
        parsed_value
    };
}

void free_parsed_for(void *ptr) {
    ParsedValue *parsedValue = ptr;
    ParsedFor *parsed_for = parsedValue->data;

    free_destructure(parsed_for->value);

    free_parsed(parsed_for->iterator);
    free(parsed_for->iterator);

    free_parsed(parsed_for->content);
    free(parsed_for->content);

    free(parsed_for);
}