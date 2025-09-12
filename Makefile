# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

BINARY = bin/argon
FLEX_TOOL = flex

SRC_DIRS = src external/xxhash external/cwalk/src external/libdye/src external/linenoise
CFILES = $(shell find $(SRC_DIRS) -name '*.c' \
    ! -path "*/tests/*" \
    ! -path "*/fuzz/*" \
    ! -path "*/cli/*" \
    ! -path "*/example*") \
 $(LEXER_C)
OBJDIR = build
OBJS = $(CFILES:%.c=$(OBJDIR)/%.o)

CFLAGS = $(ARCHFLAGS) -Wall -Wextra -Wno-unused-function -Werror=unused-result \
         -Iexternal/cwalk/include -Iexternal/libdye/include
LDFLAGS = -lgc -lgmp -lm

all: $(BINARY)

# Rule to build lexer
$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	$(FLEX_TOOL) --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

# Pattern rule for compiling .c -> .o
$(OBJDIR)/%.o: %.c $(LEXER_C) $(LEXER_H)
	@mkdir -p $(dir $@)
	gcc -O3 -c $< -o $@ $(CFLAGS)

# Link final binary
$(BINARY): $(OBJS)
	@mkdir -p bin
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) -s

native: CFLAGS += -march=native
native: $(BINARY)

debug: CFLAGS += -g
debug: $(BINARY)

full-debug: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
full-debug: $(BINARY)

optimised: CFLAGS += -fprofile-generate
optimised: $(BINARY)
	${BINARY} rand_test.ar
	$(MAKE) CFLAGS="$(CFLAGS:-fprofile-generate=-fprofile-use)" $(BINARY)

clean:
	rm -rf build bin
	rm -f $(LEXER_C) $(LEXER_H)