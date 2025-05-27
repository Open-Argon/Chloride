LEXER_SRC = src/lexer/lex.l
LEXER_C = src/lexer/lex.yy.c
LEXER_H = src/lexer/lex.yy.h

CFILES = $(shell find src -name '*.c')
BINARY = bin/cargon

all: $(BINARY)

$(LEXER_C) $(LEXER_H): $(LEXER_SRC)
	flex --header-file=$(LEXER_H) -o $(LEXER_C) $(LEXER_SRC)

$(BINARY): $(CFILES) $(LEXER_C) $(LEXER_H)
	mkdir -p bin
	gcc -O3 -o $(BINARY) $(CFILES) -lm -Wall -Wextra -Wno-unused-function

clean:
	rm -rf bin
	rm -f $(LEXER_C) $(LEXER_H)