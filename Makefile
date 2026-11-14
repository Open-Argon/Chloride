# SPDX-FileCopyrightText: 2025, 2026 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# ------------------------------------------------------------
# Configurable variables
# ------------------------------------------------------------
TARGET_OS ?= posix       # default target, can be 'posix' or 'windows'
GC        ?= boehm       # garbage collector backend: 'boehm' or 'custom'
override GC := $(strip $(GC))

ifeq ($(TARGET_OS),windows)
	CC     = x86_64-w64-mingw32-gcc
	BINARY = bin/argon.exe
	LDFLAGS = -lgmp -lm -lmpfr -lbcrypt
	ifeq ($(GC),boehm)
		LDFLAGS += -lgc
	endif
else
	CC     = gcc
	BINARY = bin/argon
	LDFLAGS = $(shell pkg-config --libs gmp mpfr) -lm -lpthread
	ifeq ($(GC),boehm)
		LDFLAGS += $(shell pkg-config --libs bdw-gc)
	endif
# 	CFLAGS  += $(shell pkg-config --cflags gmp mpfr bdw-gc)
endif

FLEX_TOOL  = flex
BUILD_DIR  = build/dev-$(GC)

# Install prefix
PREFIX	   ?= /usr/local
INSTALL_DIR	= $(PREFIX)/bin

# Source files
CFILES = \
	external/xxhash/xxhash.c \
	external/cwalk/src/cwalk.c \
	external/libdye/src/dye.c \
	$(filter-out $(LEXER_C),$(shell find src -name '*.c' -not -path 'src/gc/tests/*'))

# Include linenoise for any POSIX target (i.e., not Windows)
ifeq ($(TARGET_OS),windows)
# do nothing
else
CFILES += external/linenoise/linenoise.c
endif

LEXER_SRC = src/lexer/lex.l
LEXER_C   = src/lexer/lex.yy.c
LEXER_H   = src/lexer/lex.yy.h

# Object files
OBJFILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CFILES)) \
           $(BUILD_DIR)/$(LEXER_C:.c=.o)

# Compiler flags
CFLAGS  = $(ARCHFLAGS) -Wall -Wextra -Wno-unused-function \
          -Werror=unused-result \
          -Iexternal/cwalk/include \
          -Iexternal/libdye/include \
		  $(shell pkg-config --cflags gmp mpfr)

ifeq ($(GC),boehm)
CFLAGS += -Iexternal/bdwgc/include $(shell pkg-config --cflags bdw-gc)
else ifeq ($(GC),custom)
CFLAGS += -DARGON_GC_CUSTOM
# separate binary so both backends can be compared side by side
BINARY := $(BINARY:bin/argon%=bin/argon-customgc%)
else
$(error GC must be 'boehm' or 'custom', got '$(GC)')
endif

VERSION := $(shell git describe --tags)
CFLAGS += -DVERSION=\"$(VERSION)\"
# Strip flag (only for non-debug builds)
STRIP_FLAG = -s

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------
all: $(BINARY)

# ------------------------------------------------------------
# Lexer generation
# ------------------------------------------------------------
$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	$(FLEX_TOOL) --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

# ------------------------------------------------------------
# Compile rule
# ------------------------------------------------------------
# -MMD -MP: also write a .d file listing the headers each object includes, so
# changing a header (a struct layout, an inline function) rebuilds everything
# that uses it.
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -O3 -MMD -MP -c $< -o $@ $(CFLAGS)

-include $(OBJFILES:.o=.d)

# ------------------------------------------------------------
# Link final binary
# ------------------------------------------------------------
$(BINARY): $(LEXER_C) $(LEXER_H) $(OBJFILES)
	mkdir -p bin
	$(CC) -o $(BINARY) $(OBJFILES) $(LDFLAGS) $(CFLAGS) $(STRIP_FLAG)

# ------------------------------------------------------------
# Variants
# ------------------------------------------------------------
# Native optimizations
native: CFLAGS += -march=native -O3
native: STRIP_FLAG = -s
native: $(BINARY)

# Debug (keep symbols)
debug: CFLAGS += -g -DARGON_DEBUG
debug: STRIP_FLAG =
debug: $(BINARY)

# Full debug (keep symbols, enable ASan)
full-debug: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer -DARGON_DEBUG
full-debug: STRIP_FLAG =
full-debug: $(BINARY)

# Optimized with profiling
optimised:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -O3 -fprofile-generate" STRIP_FLAG=-s $(BINARY)
	$(BINARY) rand_test.ar
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -O3 -fprofile-use" STRIP_FLAG=-s $(BINARY)

# ------------------------------------------------------------
# Custom GC unit tests (standalone, don't need the interpreter)
# ------------------------------------------------------------
GC_TEST_FLAGS = -O1 -g -Wall -Wextra -DARGON_GC_CUSTOM -fsanitize=address

gc-test:
	@mkdir -p build/gc-test
	$(CC) $(GC_TEST_FLAGS) src/gc/tests/heap_test.c src/gc/heap.c -o build/gc-test/heap_test
	$(CC) $(GC_TEST_FLAGS) src/gc/tests/gc_test.c src/gc/gc.c src/gc/verify.c src/gc/heap.c -o build/gc-test/gc_test
	./build/gc-test/heap_test
	./build/gc-test/gc_test

.PHONY: gc-test

# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------
clean:
	rm -rf build bin
	find . -type d -name "__arcache__" -exec rm -rf {} +
	rm -f $(LEXER_C) $(LEXER_H)

# Install stuff
install: $(BINARY)
	install -d $(INSTALL_DIR)
	install -m 755 $(BINARY) $(INSTALL_DIR)/argon
uninstall:
	rm $(INSTALL_DIR)/argon