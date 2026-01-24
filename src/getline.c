/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#if defined(_WIN32) || defined(_WIN64)
#include <stdio.h>
#include <windows.h>
FILE *fmemopen(void *buf, size_t size, const char *mode) {
  if (strchr(mode, 'r') == NULL) {
    return NULL;
  }

  FILE *tmp = tmpfile();
  if (!tmp)
    return NULL;

  if (fwrite(buf, 1, size, tmp) != size) {
    fclose(tmp);
    return NULL;
  }

  rewind(tmp);

  return tmp;
}

// Only define ssize_t if it doesn't already exist
#ifndef _SSIZE_T_DEFINED
typedef long ssize_t;
#define _SSIZE_T_DEFINED
#endif

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
  if (!lineptr || !n || !stream)
    return -1;

  size_t pos = 0;
  int c;

  if (*lineptr == NULL || *n == 0) {
    *n = 128;
    *lineptr = malloc(*n);
    if (!*lineptr)
      return -1;
  }

  while ((c = fgetc(stream)) != EOF) {
    if (pos + 1 >= *n) {
      *n *= 2;
      char *tmp = realloc(*lineptr, *n);
      if (!tmp)
        return -1;
      *lineptr = tmp;
    }
    (*lineptr)[pos++] = c;
    if (c == '\n')
      break;
  }

  if (pos == 0 && c == EOF)
    return -1;

  (*lineptr)[pos] = '\0';
  return (ssize_t)pos;
}
#endif