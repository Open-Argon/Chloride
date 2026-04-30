#!/usr/bin/env bash

STDLIB_DIR=""
MAKE_ARGS=()

# Parse arguments
for arg in "$@"; do
    if [ -z "$STDLIB_DIR" ]; then
        STDLIB_DIR="$arg"
    else
        MAKE_ARGS+=("$arg")
    fi
done

# Require stdlib dir explicitly (fail fast instead of guessing)
if [ -z "$STDLIB_DIR" ]; then
    echo "Usage: $0 <stdlib-path> [make-args...]" >&2
    exit 1
fi

STDLIB_DIR="$(cd "$STDLIB_DIR" && pwd)"

for lib in "$STDLIB_DIR"/*/; do
    if [ -f "$lib/Makefile" ]; then
        echo ">>> Building $(basename "$lib")..."
        make -C "$lib" clean && \
        make -C "$lib" "${MAKE_ARGS[@]}"
        if [ $? -ne 0 ]; then
            echo "!!! Failed to build $(basename "$lib")" >&2
        fi
    fi
done