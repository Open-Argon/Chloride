# SPDX-FileCopyrightText: 2025, 2026 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# ------------------------------------------------------------
# Stdlib metadata
# ------------------------------------------------------------
LIB        := network
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
		-lws2_32

	PLATFORM_CFLAGS := \
		-DWIN32 \
		-D_WIN32_WINNT=0x0601

else ifeq ($(UNAME_S),Darwin)

	ifeq ($(origin CC),default)
	  CC      := gcc
	endif
	LIB_EXT := dylib

	SHARED_LDFLAGS := \
		-dynamiclib \
		-Wl,-install_name,@rpath/$(LIB).dylib

	PLATFORM_LDFLAGS :=
	PLATFORM_CFLAGS :=

else

	# Linux / FreeBSD
	ifeq ($(origin CC),default)
	  CC      := gcc
	endif
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
# Optional TLS support (client-side), backed by OpenSSL.
#   WITH_TLS=1 (default) - build with TLS if pkg-config finds openssl
#   WITH_TLS=0            - force-disable TLS support
# ------------------------------------------------------------
WITH_TLS ?= 1

ifeq ($(WITH_TLS),1)
	OPENSSL_CFLAGS := $(shell pkg-config --cflags openssl 2>/dev/null)
	OPENSSL_LIBS   := $(shell pkg-config --libs openssl 2>/dev/null)

	ifeq ($(OPENSSL_LIBS),)
		# pkg-config couldn't find it; fall back to plain -lssl -lcrypto and
		# hope it's on the default include/lib path.
		OPENSSL_LIBS := -lssl -lcrypto
	endif

	TLS_CFLAGS := -DNET_WITH_TLS $(OPENSSL_CFLAGS)
	TLS_LDFLAGS := $(OPENSSL_LIBS)
else
	TLS_CFLAGS :=
	TLS_LDFLAGS :=
endif

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
		$(TLS_CFLAGS) \
		$(CFLAGS) \
		-I$(ARGON_INCLUDE) \
		$(SHARED_LDFLAGS) \
		-o $@ \
		$^ \
		$(PLATFORM_LDFLAGS) \
		$(TLS_LDFLAGS)

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
