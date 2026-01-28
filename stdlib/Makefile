# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# ------------------------------------------------------------
# Target OS
# ------------------------------------------------------------
TARGET_OS ?= posix

ifeq ($(TARGET_OS),windows)
	CC        = x86_64-w64-mingw32-gcc
	LIB_EXT   = dll
	SHARED_LDFLAGS = -shared
else ifeq ($(TARGET_OS),macos)
	CC        = gcc
	LIB_EXT   = dylib
	SHARED_LDFLAGS = -dynamiclib -Wl,-install_name,@rpath/$(LIB).dylib
else
	CC        = gcc
	LIB_EXT   = so
	SHARED_LDFLAGS = -shared
endif

# ------------------------------------------------------------
# Common flags
# ------------------------------------------------------------
COMMON_CFLAGS = \
	-fPIC \
	-fvisibility=hidden \
	-Wall -Wextra \
	-Werror=unused-result

ARGON_INCLUDE ?= ../include

# ------------------------------------------------------------
# Stdlib discovery (BULLETPROOF)
# Finds: network/native → network
# ------------------------------------------------------------
STDLIBS := $(shell find . -maxdepth 2 -type d -name native | sed 's|^\./||; s|/native$$||')

# ------------------------------------------------------------
# Phony targets
# ------------------------------------------------------------
.PHONY: all clean build-lib \
        build-debug build-full-debug build-native \
        $(STDLIBS)

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------
all: $(STDLIBS)

# ------------------------------------------------------------
# Per-stdlib entry points
# ------------------------------------------------------------
$(STDLIBS):
	$(MAKE) build-lib LIB=$@

# ------------------------------------------------------------
# Build logic (runtime expansion)
# ------------------------------------------------------------
build-lib:
	@echo "==> Building stdlib: $(LIB)"
	@mkdir -p $(LIB)/native/bin

	SRC="$(shell ls $(LIB)/native/*.c 2>/dev/null)"; \
	if [ -z "$$SRC" ]; then \
		echo "error: no C sources found for stdlib '$(LIB)'"; \
		exit 1; \
	fi; \
	EXTRA_CFLAGS="$$(test -f $(LIB)/native/compiler_flags.txt && cat $(LIB)/native/compiler_flags.txt)"; \
	$(CC) \
		$(COMMON_CFLAGS) \
		$(CFLAGS) \
		$$EXTRA_CFLAGS \
		-I$(ARGON_INCLUDE) \
		$(SHARED_LDFLAGS) \
		-o $(LIB)/native/bin/$(LIB).$(LIB_EXT) \
		$$SRC

# ------------------------------------------------------------
# Build variants
# ------------------------------------------------------------
build-debug:
	$(MAKE) CFLAGS="-g" all

build-full-debug:
	$(MAKE) CFLAGS="-g -fsanitize=address -fno-omit-frame-pointer" all

build-native:
	$(MAKE) CFLAGS="-march=native" all

# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------
clean:
	rm -rf */native/bin
