# SPDX-FileCopyrightText: 2025, 2026 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# ------------------------------------------------------------
# Stdlib metadata
# ------------------------------------------------------------
LIB        := archive
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

	ifeq ($(origin CC),default)
		CC := x86_64-w64-mingw32-gcc
	endif
	LIB_EXT := dll

	SHARED_LDFLAGS := -shared
	PLATFORM_LDFLAGS := \
		-lz \
		-lws2_32 \
		-lbcrypt \
		-lole32 \
		-loleaut32 \
		-lxmllite \
		-luuid

	PLATFORM_CFLAGS :=

else ifeq ($(UNAME_S),Darwin)

	ifeq ($(origin CC),default)
		CC := clang
	endif

	LIB_EXT := dylib

	SHARED_LDFLAGS := \
		-dynamiclib \
		-Wl,-install_name,@rpath/$(LIB).dylib

	BREW_PREFIX := $(shell brew --prefix 2>/dev/null)

	PLATFORM_CFLAGS := \
		-I$(BREW_PREFIX)/opt/zstd/include \
		-I$(BREW_PREFIX)/opt/xz/include \
		-I$(BREW_PREFIX)/opt/bzip2/include \
		-I$(BREW_PREFIX)/opt/lz4/include \
		-I$(BREW_PREFIX)/opt/openssl@3/include \
		-I$(BREW_PREFIX)/opt/libxml2/include \
		-I$(BREW_PREFIX)/opt/libiconv/include

	PLATFORM_LDFLAGS := \
		-L$(BREW_PREFIX)/opt/zstd/lib \
		-L$(BREW_PREFIX)/opt/xz/lib \
		-L$(BREW_PREFIX)/opt/bzip2/lib \
		-L$(BREW_PREFIX)/opt/lz4/lib \
		-L$(BREW_PREFIX)/opt/openssl@3/lib \
		-L$(BREW_PREFIX)/opt/libxml2/lib \
		-L$(BREW_PREFIX)/opt/libiconv/lib \
		-lzstd \
		-llzma \
		-lbz2 \
		-llz4 \
		-lz \
		-lcrypto \
		-lxml2 \
		-liconv

else

	# Linux / FreeBSD
	ifeq ($(origin CC),default)
	  CC      := gcc
	endif
	LIB_EXT := so

	SHARED_LDFLAGS := -shared
	PLATFORM_LDFLAGS := -lzstd -llzma -lbz2 -lz -lcrypto -lxml2
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
# libarchive location
#   By default we build/link against the vendored copy at
#   ../../external/libarchive, same pattern regex uses for pcre2.
# ------------------------------------------------------------
LIBARCHIVE_DIR      ?= $(ARGON_INCLUDE)/../external/libarchive
LIBARCHIVE_BUILD_DIR ?= $(SRC_DIR)/libarchive/build
LIBARCHIVE_INCLUDE  ?= $(LIBARCHIVE_DIR)/libarchive

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------
SRC := $(wildcard $(SRC_DIR)/*.c) $(LIBARCHIVE_BUILD_DIR)/libarchive/libarchive.a
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
		-I$(LIBARCHIVE_INCLUDE) \
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
