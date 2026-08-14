#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 William Bell
#
# SPDX-License-Identifier: LGPL-3.0-or-later

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

ROOT_DIR="$(pwd)"

cd "$SCRIPT_DIR"

# Parse make-style arguments
TARGET_OS="linux"  # default

for arg in "$@"; do
    case "$arg" in
        TARGET_OS=*)
            TARGET_OS="${arg#TARGET_OS=}"
            ;;
    esac
done

case "$TARGET_OS" in
    linux)
        HOST_PROFILE="default"
        ;;

    linux-arm64)
        HOST_PROFILE="$ROOT_DIR/aarch64-linux-gnu.txt"
        ;;

    windows)
        HOST_PROFILE="$ROOT_DIR/mingw-x86_64.txt"
        ;;

    *)
        echo "Unsupported TARGET_OS: $TARGET_OS"
        echo "Supported targets: linux, linux-arm64, windows"
        exit 1
        ;;
esac

echo "Building archive stdlib for: $TARGET_OS"
echo "Host profile: $HOST_PROFILE"

rm -rf build
rm -rf native/bin

conan install . \
    --profile:build=default \
    --profile:host="$HOST_PROFILE" \
    --build=missing

conan build . \
    --profile:build=default \
    --profile:host="$HOST_PROFILE"

rm -rf build