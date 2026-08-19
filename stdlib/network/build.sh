#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(pwd)"

cd "$SCRIPT_DIR"

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

case "$UNAME_S:$TARGET_OS:$TARGET_ARCH" in
    Linux:posix:x86_64)
        HOST_PROFILE="default"
        ;;

    Linux:posix:amd64)
        HOST_PROFILE="default"
        ;;

    Linux:linux-arm64:arm64)
        HOST_PROFILE="$ROOT_DIR/aarch64-linux-gnu.txt"
        ;;

    Darwin:posix:arm64)
        HOST_PROFILE="$ROOT_DIR/profiles/apple/macos-arm64"
        ;;

    Darwin:posix:x86_64)
        HOST_PROFILE="$ROOT_DIR/profiles/apple/macos-x86_64"
        ;;

    *:windows:x86_64)
        HOST_PROFILE="$ROOT_DIR/mingw-x86_64.txt"
        ;;

    *:windows:amd64)
        HOST_PROFILE="$ROOT_DIR/mingw-x86_64.txt"
        ;;

    *)
        echo "Unsupported target:"
        echo "  UNAME_S=$UNAME_S"
        echo "  TARGET_OS=$TARGET_OS"
        echo "  TARGET_ARCH=$TARGET_ARCH"
        exit 1
        ;;
esac

echo "Building network stdlib for: $UNAME_S / $TARGET_OS / $TARGET_ARCH"
echo "Host profile: $HOST_PROFILE"

rm -rf build
rm -rf native/bin

conan install . \
    --profile:build=default \
    --profile:host="$HOST_PROFILE" \
    --build=missing \
    "${BUILD_ARGS[@]}"

conan build . \
    --profile:build=default \
    --profile:host="$HOST_PROFILE"

rm -rf build