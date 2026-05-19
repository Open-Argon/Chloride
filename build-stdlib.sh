#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 William Bell

set -e

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

if [ -z "$STDLIB_DIR" ]; then
    STDLIB_DIR="stdlib"
    echo "defaulting to $STDLIB_DIR"
fi

STDLIB_DIR="$(cd "$STDLIB_DIR" && pwd)"

for lib in "$STDLIB_DIR"/*/; do
    if [ -f "$lib/build.sh" ]; then
        echo ">>> Building $(basename "$lib") (via build.sh)..."
        bash "$lib/build.sh" "${MAKE_ARGS[@]}"
        if [ $? -ne 0 ]; then
            echo "!!! Failed to build $(basename "$lib")" >&2
        fi
    elif [ -f "$lib/Makefile" ]; then
        echo ">>> Building $(basename "$lib")..."
        make -C "$lib" clean && \
        make -C "$lib" "${MAKE_ARGS[@]}"
        if [ $? -ne 0 ]; then
            echo "!!! Failed to build $(basename "$lib")" >&2
        fi
    fi
done