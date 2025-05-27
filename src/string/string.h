#ifndef CLONESTRING_H
#define CLONESTRING_H

extern const char * WHITE_SPACE;

char* cloneString(char* str);

void stripString(char* str, const char* chars);

char *swap_quotes(const char *input);

char *unquote(const char *str);


#endif // CLONESTRING_H
