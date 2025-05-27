typedef struct {
    const char *filename;
    int current_column;
    // add more fields as needed
} LexerState;

int lexer();
