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

        BREW_PREFIX="$(brew --prefix)"

        ZSTD_PREFIX="$(brew --prefix zstd)"
        XZ_PREFIX="$(brew --prefix xz)"
        BZIP2_PREFIX="$(brew --prefix bzip2)"
        LZ4_PREFIX="$(brew --prefix lz4)"
        OPENSSL_PREFIX="$(brew --prefix openssl@3)"
        LIBXML2_PREFIX="$(brew --prefix libxml2)"
        LIBICONV_PREFIX="$(brew --prefix libiconv)"

        echo "Using compiler: $CC"
        echo "Homebrew prefix: $BREW_PREFIX"

        # Verify required dependencies exist.
        for prefix in \
            "$ZSTD_PREFIX" \
            "$XZ_PREFIX" \
            "$BZIP2_PREFIX" \
            "$LZ4_PREFIX" \
            "$OPENSSL_PREFIX" \
            "$LIBXML2_PREFIX" \
            "$LIBICONV_PREFIX"
        do
            if [ ! -d "$prefix" ]; then
                echo "ERROR: Required Homebrew dependency not found:"
                echo "  $prefix"
                exit 1
            fi
        done

        CMAKE_ARGS+=(
            "-DCMAKE_C_COMPILER=$CC"
            "-DCMAKE_SYSTEM_NAME=Darwin"
            "-DCMAKE_SYSTEM_PROCESSOR=$(uname -m)"

            "-DCMAKE_PREFIX_PATH=$BREW_PREFIX"

            "-DCMAKE_INCLUDE_PATH=$ZSTD_PREFIX/include;$XZ_PREFIX/include;$BZIP2_PREFIX/include;$LZ4_PREFIX/include;$OPENSSL_PREFIX/include;$LIBXML2_PREFIX/include;$LIBICONV_PREFIX/include"

            "-DCMAKE_LIBRARY_PATH=$ZSTD_PREFIX/lib;$XZ_PREFIX/lib;$BZIP2_PREFIX/lib;$LZ4_PREFIX/lib;$OPENSSL_PREFIX/lib;$LIBXML2_PREFIX/lib;$LIBICONV_PREFIX/lib"

            "-DIconv_INCLUDE_DIR=$LIBICONV_PREFIX/include"
            "-DIconv_LIBRARY=$LIBICONV_PREFIX/lib/libiconv.dylib"

            "-DENABLE_WERROR=OFF"

            "-DENABLE_ZLIB=ON"
            "-DENABLE_ZSTD=ON"
            "-DENABLE_LZMA=ON"
            "-DENABLE_BZIP2=ON"
            "-DENABLE_LZ4=ON"
            "-DENABLE_OPENSSL=ON"
            "-DENABLE_LIBXML2=ON"
        )
        ;;

    windows)
        CC="${CC:-x86_64-w64-mingw32-gcc}"

        MINGW_TARGET="$("$CC" -dumpmachine)"

        echo "Using compiler: $CC"
        echo "MinGW target:  $MINGW_TARGET"

        # --------------------------------------------------------
        # Determine MinGW prefix
        #
        # Debian:
        #   /usr/x86_64-w64-mingw32/include
        #   /usr/x86_64-w64-mingw32/lib
        #
        # Fedora:
        #   /usr/x86_64-w64-mingw32/sys-root/mingw/include
        #   /usr/x86_64-w64-mingw32/sys-root/mingw/lib
        # --------------------------------------------------------

        MINGW_ROOT="/usr/$MINGW_TARGET"

        if [ -d "$MINGW_ROOT/sys-root/mingw" ]; then
            MINGW_PREFIX="$MINGW_ROOT/sys-root/mingw"
        elif [ -d "$MINGW_ROOT" ]; then
            MINGW_PREFIX="$MINGW_ROOT"
        else
            echo "ERROR: Cannot determine MinGW prefix"
            echo "  compiler: $CC"
            echo "  target:   $MINGW_TARGET"
            echo "  expected:"
            echo "    $MINGW_ROOT"
            echo "    $MINGW_ROOT/sys-root/mingw"
            exit 1
        fi

        echo "MinGW root:    $MINGW_ROOT"
        echo "MinGW prefix:  $MINGW_PREFIX"

        # --------------------------------------------------------
        # Locate zlib
        # --------------------------------------------------------

        ZLIB_INCLUDE_DIR=""
        ZLIB_LIBRARY=""

        for dir in \
            "$MINGW_PREFIX/include" \
            "$MINGW_ROOT/include"
        do
            if [ -f "$dir/zlib.h" ]; then
                ZLIB_INCLUDE_DIR="$dir"
                break
            fi
        done

        for lib in \
            "$MINGW_PREFIX/lib/libz.a" \
            "$MINGW_PREFIX/lib/libz.dll.a" \
            "$MINGW_ROOT/lib/libz.a" \
            "$MINGW_ROOT/lib/libz.dll.a"
        do
            if [ -f "$lib" ]; then
                ZLIB_LIBRARY="$lib"
                break
            fi
        done

        if [ -z "$ZLIB_INCLUDE_DIR" ]; then
            echo "ERROR: MinGW zlib development headers not found"
            echo "  compiler: $CC"
            echo "  target:   $MINGW_TARGET"
            echo "  prefix:   $MINGW_PREFIX"
            echo ""
            echo "Install the MinGW zlib development package:"
            echo "  Debian: libz-mingw-w64-dev"
            echo "  Fedora: mingw32-zlib"
            exit 1
        fi

        if [ -z "$ZLIB_LIBRARY" ]; then
            echo "ERROR: MinGW zlib library not found"
            echo "  compiler: $CC"
            echo "  target:   $MINGW_TARGET"
            echo "  prefix:   $MINGW_PREFIX"
            exit 1
        fi

        echo "zlib include:  $ZLIB_INCLUDE_DIR"
        echo "zlib library:  $ZLIB_LIBRARY"

        # --------------------------------------------------------
        # CMake cross compilation
        # --------------------------------------------------------

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
            "-DZLIB_INCLUDE_DIR=$ZLIB_INCLUDE_DIR"
            "-DZLIB_LIBRARY=$ZLIB_LIBRARY"

            "-DENABLE_ICONV=OFF"
            "-DENABLE_LIBXML2=OFF"
            "-DENABLE_OPENSSL=OFF"
            "-DENABLE_MBEDTLS=OFF"
            "-DENABLE_NETTLE=OFF"

            "-DLIBMD_FOUND:BOOL=FALSE"

            "-DHAVE_XMLLITE_H=OFF"

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