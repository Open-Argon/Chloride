#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Always clean first. The stdlib is packaged after the build, so stale
# artifacts must never be allowed to survive between builds.
make -C "$SCRIPT_DIR" clean
rm -rf "$SCRIPT_DIR/native/libarchive"

# ------------------------------------------------------------
# Determine target OS
# ------------------------------------------------------------

TARGET_OS=""

for arg in "$@"; do
    case "$arg" in
        TARGET_OS=*)
            TARGET_OS="${arg#TARGET_OS=}"
            ;;
    esac
done

if [ -z "$TARGET_OS" ]; then
    case "$(uname -s)" in
        Linux)
            TARGET_OS="linux"
            ;;
        Darwin)
            TARGET_OS="macos"
            ;;
        *)
            echo "Unsupported host OS: $(uname -s)"
            exit 1
            ;;
    esac
fi

echo "Building for target: $TARGET_OS"

# ------------------------------------------------------------
# Configure compiler / CMake toolchain
# ------------------------------------------------------------

CMAKE_ARGS=()

case "$TARGET_OS" in
    linux)
        CC="${CC:-gcc}"

        CMAKE_ARGS+=(
            "-DCMAKE_C_COMPILER=$CC"
            "-DENABLE_WERROR=OFF"
        )
        ;;

    macos)
        CC="${CC:-clang}"

        CMAKE_ARGS+=(
            "-DCMAKE_C_COMPILER=$CC"
            "-DENABLE_WERROR=OFF"
        )
        ;;

    windows)
        CC="${CC:-x86_64-w64-mingw32-gcc}"

        MINGW_TARGET="$("$CC" -dumpmachine)"
        MINGW_ROOT="/usr/$MINGW_TARGET"

        if [ ! -d "$MINGW_ROOT" ]; then
            echo "ERROR: MinGW target root not found"
            echo "  compiler: $CC"
            echo "  target:   $MINGW_TARGET"
            echo "  root:     $MINGW_ROOT"
            exit 1
        fi

        # Debian/Ubuntu MinGW-w64 layout:
        #
        #   /usr/x86_64-w64-mingw32/include
        #   /usr/x86_64-w64-mingw32/lib
        #
        # Some distributions instead use:
        #
        #   /usr/x86_64-w64-mingw32/sys-root/mingw
        #
        # Handle both.
        if [ -d "$MINGW_ROOT/sys-root/mingw/include" ]; then
            MINGW_PREFIX="$MINGW_ROOT/sys-root/mingw"
        elif [ -d "$MINGW_ROOT/include" ]; then
            MINGW_PREFIX="$MINGW_ROOT"
        else
            echo "ERROR: Cannot determine MinGW target prefix"
            echo "  compiler: $CC"
            echo "  target:   $MINGW_TARGET"
            echo "  root:     $MINGW_ROOT"
            exit 1
        fi

        echo "Using compiler: $CC"
        echo "MinGW target:  $MINGW_TARGET"
        echo "MinGW prefix:  $MINGW_PREFIX"

        CMAKE_ARGS+=(
            "-DCMAKE_SYSTEM_NAME=Windows"
            "-DCMAKE_SYSTEM_PROCESSOR=x86_64"
            "-DCMAKE_C_COMPILER=$CC"
            "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"

            "-DCMAKE_FIND_ROOT_PATH=$MINGW_PREFIX"
            "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"
            "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
            "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
            "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY"

            "-DENABLE_ZLIB=ON"
            "-DZLIB_INCLUDE_DIR=$MINGW_PREFIX/include"
            "-DZLIB_LIBRARY=$MINGW_PREFIX/lib/libz.dll.a"

            "-DENABLE_OPENSSL=OFF"
            "-DENABLE_MBEDTLS=OFF"
            "-DENABLE_NETTLE=OFF"
            "-DLIBMD_FOUND:BOOL=FALSE"
            "-DENABLE_WERROR=OFF"
            "-DENABLE_UNZIP=OFF"
        )
        ;;

    *)
        echo "Unsupported TARGET_OS: $TARGET_OS"
        echo "Supported targets: linux, macos, windows"
        exit 1
        ;;
esac

echo "Using compiler: $CC"

# ------------------------------------------------------------
# Configure and build libarchive
# ------------------------------------------------------------

(
    cd "./external/libarchive"

    cmake -G Ninja \
        "${CMAKE_ARGS[@]}" \
        -DENABLE_ACL=OFF \
        -DENABLE_XATTR=OFF \
        -DENABLE_TEST=OFF \
        -DENABLE_TAR=OFF \
        -DENABLE_CPIO=OFF \
        -DENABLE_CAT=OFF \
        -DENABLE_INSTALL=OFF \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -B "$SCRIPT_DIR/native/libarchive/build"

    cmake --build "$SCRIPT_DIR/native/libarchive/build"
)

# ------------------------------------------------------------
# Build the rest of the stdlib
# ------------------------------------------------------------

make -C "$SCRIPT_DIR" "$@"

# ------------------------------------------------------------
# Remove temporary libarchive build artifacts
# ------------------------------------------------------------

rm -rf "$SCRIPT_DIR/native/libarchive"