TARGET_ARGS=""
if [ "$1" = "--windows" ]; then
    TARGET_ARGS="TARGET_OS=windows"
fi

for lib in $(dirname "$0")/stdlib/*/; do
    if [ -f "$lib/Makefile" ]; then
        echo ">>> Building $(basename "$lib")..."
        make -C "$lib" clean && make -C "$lib" -j $TARGET_ARGS
        if [ $? -ne 0 ]; then
            echo "!!! Failed to build $(basename "$lib")" >&2
        fi
    fi
done