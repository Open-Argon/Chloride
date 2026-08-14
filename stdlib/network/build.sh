#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

make -C "$SCRIPT_DIR" clean

# ------------------------------------------------------------
# Parse make-style arguments
# ------------------------------------------------------------

TARGET_OS="linux"

for arg in "$@"; do
    case "$arg" in
        TARGET_OS=*)
            TARGET_OS="${arg#TARGET_OS=}"
            ;;
    esac
done

# ------------------------------------------------------------
# Set compiler
# ------------------------------------------------------------

if [ "$TARGET_OS" = "windows" ]; then
    CC="${CC:-x86_64-w64-mingw32-gcc}"
else
    CC="${CC:-gcc}"
fi

echo "Building for $TARGET_OS using $CC..."

# ------------------------------------------------------------
# Windows OpenSSL
# ------------------------------------------------------------

if [ "$TARGET_OS" = "windows" ]; then

    OPENSSL_VERSION="1.1.1w"
    OPENSSL_PREFIX="$SCRIPT_DIR/native/openssl-mingw64"
    OPENSSL_ARCHIVE="/tmp/openssl-${OPENSSL_VERSION}.tar.gz"
    OPENSSL_SOURCE="/tmp/openssl-${OPENSSL_VERSION}"

    if [ ! -f "$OPENSSL_PREFIX/include/openssl/ssl.h" ]; then

        echo "==> Building OpenSSL ${OPENSSL_VERSION} for Windows"

        rm -rf "$OPENSSL_SOURCE"
        rm -f "$OPENSSL_ARCHIVE"

        curl -L \
            -o "$OPENSSL_ARCHIVE" \
            "https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz"

        tar -xzf "$OPENSSL_ARCHIVE" -C /tmp

        cd "$OPENSSL_SOURCE"

        ./Configure \
            mingw64 \
            --cross-compile-prefix=x86_64-w64-mingw32- \
            --prefix="$OPENSSL_PREFIX" \
            --openssldir="$OPENSSL_PREFIX/ssl" \
            no-shared

        make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
        make install_sw

        cd "$SCRIPT_DIR"
    fi

    echo "Using MinGW OpenSSL:"
    echo "  prefix: $OPENSSL_PREFIX"

    if [ ! -f "$OPENSSL_PREFIX/include/openssl/ssl.h" ]; then
        echo "ERROR: MinGW OpenSSL installation failed"
        exit 1
    fi

    if [ ! -f "$OPENSSL_PREFIX/lib/libssl.a" ]; then
        echo "ERROR: MinGW OpenSSL libssl.a not found"
        exit 1
    fi

    if [ ! -f "$OPENSSL_PREFIX/lib/libcrypto.a" ]; then
        echo "ERROR: MinGW OpenSSL libcrypto.a not found"
        exit 1
    fi

    export OPENSSL_MINGW_PREFIX="$OPENSSL_PREFIX"
fi

# ------------------------------------------------------------
# Build stdlib
# ------------------------------------------------------------

cd "$SCRIPT_DIR"

make \
    CC="$CC" \
    "$@"