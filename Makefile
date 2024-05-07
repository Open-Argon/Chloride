build:
	mkdir -p bin
	gcc -O3 -o bin/cargon $(shell find src -name '*.c') -lm -Wall -Wextra -Werror