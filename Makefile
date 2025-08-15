# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Default FLEX tool
BINARY = bin/argon
FLEX_TOOL = flex

# If target is "windows", override FLEX_TOOL
ifeq ($(MAKECMDGOALS),windows)
	BINARY = bin/argon.exe
    FLEX_TOOL = win_flex
endif

LEXER_SRC = src/lexer/lex.l
LEXER_C = src/lexer/lex.yy.c
LEXER_H = src/lexer/lex.yy.h

CFILES = external/xxhash/xxhash.c external/cwalk/src/cwalk.c external/libdye/src/dye.c $(shell find src -name '*.c')
CFLAGS = $(ARCHFLAGS) -lm -lgc -lgmp -Wall -Wextra -Wno-unused-function -Werror=unused-result -Iexternal/cwalk/include -Iexternal/libdye/include

all: $(BINARY)

windows: $(BINARY)

$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	$(FLEX_TOOL) --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

$(BINARY): $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -o $(BINARY) $(CFILES) $(CFLAGS) -s

native: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -march=native -o $(BINARY) $(CFILES) $(CFLAGS)

debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O3 -o $(BINARY) $(CFILES) $(CFLAGS)

full-debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O0 -fsanitize=address -fno-omit-frame-pointer -o $(BINARY) $(CFILES) $(CFLAGS)

optimised: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -fprofile-generate -o $(BINARY) $(CFILES) $(CFLAGS)
	${BINARY} rand_test.ar
	gcc -O3 -fprofile-use -o $(BINARY) $(CFILES) $(CFLAGS)
	

clean:
	rm -rf build bin
	rm -f $(LEXER_C) $(LEXER_H)