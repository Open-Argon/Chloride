#ifndef PARSE_DESTRUCTURE_H
#define PARSE_DESTRUCTURE_H

#include <stddef.h>
#include "../parser.h"

typedef struct Destructure Destructure;

typedef enum {
    DESTRUCTURE_IDENTIFIER,
    DESTRUCTURE_INDEX,
    DESTRUCTURE_KEY
} DESTRUCTURE_TYPE;

typedef struct {
    char *name;
} DestructureIdentifier;

typedef struct {
    size_t length;
    Destructure *rest;
    Destructure **items;
} DestructureIndex;

typedef struct {
    ParsedValue *key;
    Destructure *destructure;
} DestructureKeyItem;

typedef struct {
    size_t length;
    Destructure *rest;
    DestructureKeyItem *items;
} DestructureKey;

struct Destructure {
    DESTRUCTURE_TYPE type;

    union {
        DestructureIdentifier identifier;
        DestructureIndex index;
        DestructureKey key;
    };
};

Destructure *parse_destructure(
    char *file,
    DArray *tokens,
    size_t *index,
    ArErr *err
);

void free_destructure(Destructure *destructure);

#endif