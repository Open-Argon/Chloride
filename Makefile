LEXER_SRC = src/lexer/lex.l
LEXER_C = src/lexer/lex.yy.c
LEXER_H = src/lexer/lex.yy.h

CFILES = $(shell find src -name '*.c')
CFLAGS = $(ARCHFLAGS) -lm -lgc -lgmp -Wall -Wextra -Wno-unused-function
BINARY = bin/argon

all: $(BINARY)

$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	flex --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

$(BINARY): $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -o $(BINARY) $(CFILES) $(CFLAGS) -s

debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O0 -o $(BINARY) $(CFILES) $(CFLAGS)

full-debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O0 -fsanitize=address -fno-omit-frame-pointer -o $(BINARY) $(CFILES) $(CFLAGS)

optimised: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -fprofile-generate -o $(BINARY) $(CFILES) $(CFLAGS)
	${BINARY} test.ar
	gcc -O3 -fprofile-use -o $(BINARY) $(CFILES) $(CFLAGS)
	

clean:
	rm -rf build bin
	rm -f $(LEXER_C) $(LEXER_H)