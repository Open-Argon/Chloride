#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

make -C $SCRIPT_DIR clean

# Parse make-style arguments
TARGET_OS="linux"  # default

for arg in "$@"; do
    case "$arg" in
        TARGET_OS=*)
            TARGET_OS="${arg#TARGET_OS=}"
            ;;
    esac
done

# Set compiler based on target
if [ "$TARGET_OS" = "windows" ]; then
    CC="x86_64-w64-mingw32-gcc"
else
    CC="gcc"
fi

echo "Building for $TARGET_OS using $CC..."

(cd ./external/pcre2 && \
    cmake -G Ninja \
    -DPCRE2_SUPPORT_JIT=ON \
    -DCMAKE_C_COMPILER=$CC \
    -DPCRE2_BUILD_PCRE2GREP=OFF \
    -DPCRE2_BUILD_TESTS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    -B $SCRIPT_DIR/native/pcre2/build && \
    cmake --build $SCRIPT_DIR/native/pcre2/build)

(cd $SCRIPT_DIR && make $@)