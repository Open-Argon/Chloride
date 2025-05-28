#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "memory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char* read_file_as_text(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Seek to the end to find the file size
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);  // Go back to the beginning

    // Allocate buffer (+1 for null terminator)
    char *buffer = malloc(length + 1);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the whole file into the buffer
    size_t read_size = fread(buffer, 1, length, file);
    buffer[read_size] = '\0';  // Null-terminate

    fclose(file);
    return buffer;
}

int main() {
    ar_memory_init();
    const char * path = "test.ar";

    char *content = read_file_as_text(path);
    LinkedList* tokens = create_list(sizeof(Token));
    if (!content) return 1;

    LexerState state = {
        path,
        content,
        1,
        tokens
    };
    lexer(state);

    LinkedList * parsed = create_list(sizeof(TaggedValue));

    parser(parsed, tokens, false);

    Node *current = parsed->head;
    while (current) {
        printf("%s\n", (char*)((TaggedValue*)current->data)->data);
        current = current->next;
    }

    return 0;
}
