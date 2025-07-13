/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../memory.h"

const char *WHITE_SPACE = " \t\n\r\f\v";

char *cloneString(char *str) {
  if (str == NULL) {
    return NULL;
  }

  size_t len = strlen(str);
  char *clone = checked_malloc((len + 1) * sizeof(char));

  if (clone == NULL) {
    return NULL;
  }

  strcpy(clone, str);
  return clone;
}

void stripString(char *str, const char *chars) {
  if (str == NULL || chars == NULL) {
    return;
  }

  size_t len = strlen(str);
  size_t charsLen = strlen(chars);

  if (len == 0 || charsLen == 0) {
    return;
  }
  size_t i = 0;
  while (i < len) {
    if (strchr(chars, str[i]) == NULL) {
      break;
    }
    i++;
  }

  if (i > 0) {
    memmove(str, str + i, len - i + 1);
  }
  size_t j = len - i - 1;
  while (j > 0) {
    if (strchr(chars, str[j]) == NULL) {
      break;
    }
    j--;
  }

  if (j < len) {
    str[j + 1] = '\0';
  }

  str = realloc(str, (j + 1) * sizeof(char));

  return;
}
