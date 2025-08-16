# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Default FLEX tool
BINARY = bin/argon
FLEX_TOOL = flex

CFILES = external/xxhash/xxhash.c external/cwalk/src/cwalk.c external/libdye/src/dye.c $(shell find src -name '*.c')

LEXER_SRC = src/lexer/lex.l
LEXER_C = src/lexer/lex.yy.c
LEXER_H = src/lexer/lex.yy.h
CFLAGS = $(ARCHFLAGS) -lm -lgc -lgmp -Wall -Wextra -Wno-unused-function -Werror=unused-result -Iexternal/cwalk/include -Iexternal/libdye/include
LDFLAGS = -lgc -lgmp -lm

ifeq ($(MAKECMDGOALS),windows)
	LDFLAGS = -Wl,-Bstatic -lgc -lgmp -Wl,-Bdynamic -lm
endif

all: $(BINARY)


$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	$(FLEX_TOOL) --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

$(BINARY): $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -o $(BINARY) $(CFILES) $(CFLAGS) ${LDFLAGS} -s

windows: $(CFILES) $(LEXER_C) $(LEXER_H)
	(echo -n "external/xxhash/xxhash.c " ; \
	 echo -n "external/cwalk/src/cwalk.c " ; \
	 echo -n "external/libdye/src/dye.c " ; \
	 find src -name '*.c' -print0 | xargs -0 echo -n) > sources.txt
	mkdir -p bin
	gcc -O3 -march=native -o $(BINARY) @sources.txt $(CFLAGS) -lbcrypt

native: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -march=native -o $(BINARY) $(CFILES) $(CFLAGS) ${LDFLAGS}

debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O3 -o $(BINARY) $(CFILES) $(CFLAGS)

full-debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O0 -fsanitize=address -fno-omit-frame-pointer -o $(BINARY) $(CFILES) $(CFLAGS) ${LDFLAGS}

optimised: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -fprofile-generate -o $(BINARY) $(CFILES) $(CFLAGS) ${LDFLAGS}
	${BINARY} rand_test.ar
	gcc -O3 -fprofile-use -o $(BINARY) $(CFILES) $(CFLAGS) ${LDFLAGS}
	

clean:
	rm -rf build bin
	rm -f $(LEXER_C) $(LEXER_H)