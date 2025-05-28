#include "string.h"
#include "../../lexer/token.h"

#include <cjson/cJSON.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *swap_quotes(char *input) {
  size_t len = strlen(input);
  char *result = malloc(len + 1);
  if (!result)
    return NULL;

  for (size_t i = 0; i < len; ++i) {
    if (input[i] == '"')
      result[i] = '\'';
    else if (input[i] == '\'')
      result[i] = '"';
    else
      result[i] = input[i];
  }
  result[len] = '\0';
  return result;
}

char *unquote(char *str) {
  if (*str == '\0')
    return NULL;

  char quote = str[0];
  char *swapped = NULL;
  char *unescaped = NULL;

  if (quote == '\'') {
    swapped = swap_quotes(str);
    if (!swapped)
      return NULL;
    str = swapped;
  }

  cJSON *json = cJSON_Parse(str);
  if (!json || !cJSON_IsString(json)) {
    if (swapped)
      free(swapped);
    return NULL;
  }

  // Copy unescaped string before freeing JSON object
  const char *decoded = cJSON_GetStringValue(json);
  if (!decoded) {
    cJSON_Delete(json);
    if (swapped)
      free(swapped);
    return NULL;
  }

  unescaped = strdup(decoded);
  cJSON_Delete(json);
  if (swapped)
    free(swapped);

  // If input was single-quoted, swap quotes back in the output
  if (quote == '\'') {
    char *final = swap_quotes(unescaped);
    free(unescaped);
    return final;
  }

  return unescaped;
}

TaggedValue parse_string(Token token) {
  return (TaggedValue){
    AST_STRING,
    unquote(token.value),
  };
}