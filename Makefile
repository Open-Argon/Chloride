# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# ------------------------------------------------------------
# Configurable variables
# ------------------------------------------------------------
TARGET_OS ?= posix       # default target, can be 'posix' or 'windows'

ifeq ($(TARGET_OS),windows)
	CC     = x86_64-w64-mingw32-gcc
	BINARY = bin/argon.exe
	LDFLAGS = -lgc -lgmp -lm -lmpfr -lbcrypt
else
	CC     = gcc
	BINARY = bin/argon
	LDFLAGS = -lgc -lgmp -lm -lmpfr
endif

FLEX_TOOL  = flex
BUILD_DIR  = build/dev

# Source files
CFILES = \
	external/xxhash/xxhash.c \
	external/cwalk/src/cwalk.c \
	external/libdye/src/dye.c \
	$(filter-out $(LEXER_C),$(shell find src -name '*.c'))

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
          -Iexternal/libdye/include

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
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -O3 -c $< -o $@ $(CFLAGS)

# ------------------------------------------------------------
# Link final binary
# ------------------------------------------------------------
$(BINARY): $(LEXER_C) $(LEXER_H) $(OBJFILES)
	mkdir -p bin
	$(CC) -O3 -o $(BINARY) $(OBJFILES) $(LDFLAGS) -s $(CFLAGS)

# ------------------------------------------------------------
# Variants
# ------------------------------------------------------------
native: CFLAGS += -march=native
native: $(BINARY)

debug: CFLAGS += -g
debug: $(BINARY)

full-debug: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
full-debug: $(BINARY)

optimised:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -O3 -fprofile-generate" $(BINARY)
	$(BINARY) rand_test.ar
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -O3 -fprofile-use" $(BINARY)

# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------
clean:
	rm -rf build bin
	rm -f $(LEXER_C) $(LEXER_H)