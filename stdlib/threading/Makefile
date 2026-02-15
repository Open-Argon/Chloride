# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# ------------------------------------------------------------
# Stdlib metadata
# ------------------------------------------------------------
LIB        := threading
SRC_DIR    := native
BIN_DIR    := native/bin

# ------------------------------------------------------------
# Target OS
#   posix   = Linux / FreeBSD / macOS
#   windows = Windows (via MinGW-w64)
# ------------------------------------------------------------
TARGET_OS ?= posix
UNAME_S   := $(shell uname -s)

# ------------------------------------------------------------
# Toolchain & platform specifics
# ------------------------------------------------------------
ifeq ($(TARGET_OS),windows)

	CC      := x86_64-w64-mingw32-gcc
	LIB_EXT := dll

	SHARED_LDFLAGS := -shared
	PLATFORM_LDFLAGS := \
		-lws2_32

	PLATFORM_CFLAGS := \
		-DWIN32 \
		-D_WIN32_WINNT=0x0601

else ifeq ($(UNAME_S),Darwin)

	CC      := gcc
	LIB_EXT := dylib

	SHARED_LDFLAGS := \
		-dynamiclib \
		-Wl,-install_name,@rpath/$(LIB).dylib

	PLATFORM_LDFLAGS :=
	PLATFORM_CFLAGS :=

else

	# Linux / FreeBSD
	CC      := gcc
	LIB_EXT := so

	SHARED_LDFLAGS := -shared
	PLATFORM_LDFLAGS :=
	PLATFORM_CFLAGS :=

endif

# ------------------------------------------------------------
# Common flags
# ------------------------------------------------------------
COMMON_CFLAGS := \
	-fPIC \
	-fvisibility=hidden \
	-Wall -Wextra \
	-Werror=unused-result

ARGON_INCLUDE ?= ../../include

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------
SRC := $(wildcard $(SRC_DIR)/*.c)
OUT := $(BIN_DIR)/$(LIB).$(LIB_EXT)

# ------------------------------------------------------------
# Phony targets
# ------------------------------------------------------------
.PHONY: all debug full-debug native clean

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------
all: $(OUT)

# ------------------------------------------------------------
# Build rule
# ------------------------------------------------------------
$(OUT): $(SRC)
	@echo "==> Building stdlib: $(LIB) ($(TARGET_OS))"
	@mkdir -p $(BIN_DIR)
	$(CC) \
		$(COMMON_CFLAGS) \
		$(PLATFORM_CFLAGS) \
		$(CFLAGS) \
		-I$(ARGON_INCLUDE) \
		$(SHARED_LDFLAGS) \
		-o $@ \
		$^ \
		$(PLATFORM_LDFLAGS)

# ------------------------------------------------------------
# Build variants
# ------------------------------------------------------------
debug:
	$(MAKE) CFLAGS="-g" all

full-debug:
	$(MAKE) CFLAGS="-g -fsanitize=address -fno-omit-frame-pointer" all

native:
	$(MAKE) CFLAGS="-march=native" all

# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------
clean:
	rm -rf $(BIN_DIR)
