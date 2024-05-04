build:
	mkdir -p bin
	clang -O3 -o bin/cargon $(shell find src -name '*.c') -lncurses -Wall -Wextra -Werror