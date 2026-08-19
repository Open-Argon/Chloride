#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

make -C "$SCRIPT_DIR" clean

TARGET_OS="${TARGET_OS:-posix}"
UNAME_S="${UNAME_S:-$(uname -s)}"
TARGET_ARCH="${TARGET_ARCH:-$(uname -m)}"

# Parse make-style arguments
for arg in "$@"; do
    case "$arg" in
        TARGET_OS=*)
            TARGET_OS="${arg#TARGET_OS=}"
            ;;
        UNAME_S=*)
            UNAME_S="${arg#UNAME_S=}"
            ;;
        TARGET_ARCH=*)
            TARGET_ARCH="${arg#TARGET_ARCH=}"
            ;;
    esac
done

# Set compiler based on target
if [ -n "${CC:-}" ]; then
    :
elif [ "$UNAME_S" = "Darwin" ] && [ "$TARGET_OS" = "posix" ]; then
    case "$TARGET_ARCH" in
        arm64)
            CC="arm64-apple-darwin24.5-clang"
            ;;
        x86_64)
            CC="x86_64-apple-darwin24.5-clang"
            ;;
        *)
            echo "Unsupported Darwin architecture: $TARGET_ARCH"
            exit 1
            ;;
    esac
elif [ "$TARGET_OS" = "windows" ]; then
    case "$TARGET_ARCH" in
        x86_64|amd64)
            CC="x86_64-w64-mingw32-gcc"
            ;;
        *)
            echo "Unsupported Windows architecture: $TARGET_ARCH"
            exit 1
            ;;
    esac
elif [ "$TARGET_OS" = "linux-arm64" ]; then
    CC="aarch64-linux-gnu-gcc"
else
    CC="gcc"
fi

echo "Building for $UNAME_S / $TARGET_OS / $TARGET_ARCH using $CC..."

(cd "./external/pcre2" && \
    cmake -G Ninja \
    -DPCRE2_SUPPORT_JIT=ON \
    -DCMAKE_C_COMPILER="$CC" \
    -DPCRE2_BUILD_PCRE2GREP=OFF \
    -DPCRE2_BUILD_TESTS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    -B "$SCRIPT_DIR/native/pcre2/build" && \
    cmake --build "$SCRIPT_DIR/native/pcre2/build")

(cd "$SCRIPT_DIR" && make "$@")

rm -rf "$SCRIPT_DIR/native/pcre2"