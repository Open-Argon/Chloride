# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

BINARY     = bin/argon
FLEX_TOOL  = flex
BUILD_DIR  = build/dev

# Source files
CFILES = \
	external/xxhash/xxhash.c \
	external/cwalk/src/cwalk.c \
	external/libdye/src/dye.c \
	external/linenoise/linenoise.c \
	$(filter-out $(LEXER_C),$(shell find src -name '*.c'))

LEXER_SRC = src/lexer/lex.l
LEXER_C   = src/lexer/lex.yy.c
LEXER_H   = src/lexer/lex.yy.h

# Object files (mirrors source tree)
OBJFILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CFILES)) \
           $(BUILD_DIR)/$(LEXER_C:.c=.o)

CFLAGS  = $(ARCHFLAGS) -Wall -Wextra -Wno-unused-function \
          -Werror=unused-result \
          -Iexternal/cwalk/include \
          -Iexternal/libdye/include

LDFLAGS = -lgc -lgmp -lm

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
# Compile rule (parallel-safe)
# ------------------------------------------------------------
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	gcc -O3 -c $< -o $@ $(CFLAGS)

# ------------------------------------------------------------
# Link final binary
# ------------------------------------------------------------
$(BINARY): $(LEXER_C) $(LEXER_H) $(OBJFILES)
	mkdir -p bin
	gcc -O3 -o $(BINARY) $(OBJFILES) $(LDFLAGS) -s $(CFLAGS)

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
