LEXER_SRC = src/lexer/lex.l
LEXER_C = src/lexer/lex.yy.c
LEXER_H = src/lexer/lex.yy.h

CFILES = $(shell find src -name '*.c')
CFLAGS = -lm -lcjson -Wall -Wextra -Wno-unused-function -s
BINARY = bin/argon

all: $(BINARY)

$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	flex --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

$(BINARY): $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -o $(BINARY) $(CFILES) $(CFLAGS)

debug: $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -g -O0 -o $(BINARY) $(CFILES) $(CFLAGS)

clean:
	rm -rf bin
	rm -f $(LEXER_C) $(LEXER_H)