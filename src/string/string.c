#include "string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

char *swap_quotes(const char *input) {
    size_t len = strlen(input);
    char *result = malloc(len + 1);
    if (!result) return NULL;

    for (size_t i = 0; i < len; ++i) {
        if (input[i] == '"') result[i] = '\'';
        else if (input[i] == '\'') result[i] = '"';
        else result[i] = input[i];
    }
    result[len] = '\0';
    return result;
}

char *unquote(const char *str) {
    if (*str == '\0') return NULL;

    char quote = str[0];
    char *swapped = NULL;
    char *unescaped = NULL;

    if (quote == '\'') {
        swapped = swap_quotes(str);
        if (!swapped) return NULL;
        str = swapped;
    }

    cJSON *json = cJSON_Parse(str);
    if (!json || !cJSON_IsString(json)) {
        if (swapped) free(swapped);
        return NULL;
    }

    // Copy unescaped string before freeing JSON object
    const char *decoded = cJSON_GetStringValue(json);
    if (!decoded) {
        cJSON_Delete(json);
        if (swapped) free(swapped);
        return NULL;
    }

    unescaped = strdup(decoded);
    cJSON_Delete(json);
    if (swapped) free(swapped);

    // If input was single-quoted, swap quotes back in the output
    if (quote == '\'') {
        char *final = swap_quotes(unescaped);
        free(unescaped);
        return final;
    }

    return unescaped;
}

const char *WHITE_SPACE = " \t\n\r\f\v";

char *cloneString(char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    size_t len = strlen(str);
    char *clone = malloc((len + 1) * sizeof(char));

    if (clone == NULL)
    {
        return NULL;
    }

    strcpy(clone, str);
    return clone;
}

void stripString(char *str, const char *chars)
{
    if (str == NULL || chars == NULL)
    {
        return;
    }

    size_t len = strlen(str);
    size_t charsLen = strlen(chars);

    if (len == 0 || charsLen == 0)
    {
        return;
    }
    size_t i = 0;
    while (i < len)
    {
        if (strchr(chars, str[i]) == NULL)
        {
            break;
        }
        i++;
    }

    if (i > 0)
    {
        memmove(str, str + i, len - i + 1);
    }
    size_t j = len-i - 1;
    while (j > 0)
    {
        if (strchr(chars, str[j]) == NULL)
        {
            break;
        }
        j--;
    }

    if (j < len)
    {
        str[j + 1] = '\0';
    }

    str = realloc(str, (j + 1) * sizeof(char));

    return;
}
