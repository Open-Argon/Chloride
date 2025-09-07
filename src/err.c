/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "err.h"
#include "../external/libdye/include/dye.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
  if (lineptr == NULL || n == NULL || stream == NULL) {
    return -1;
  }

  size_t pos = 0;
  int c;

  if (*lineptr == NULL || *n == 0) {
    *n = 128; // initial buffer size
    *lineptr = malloc(*n);
    if (*lineptr == NULL) {
      return -1;
    }
  }

  while ((c = fgetc(stream)) != EOF) {
    // Resize buffer if needed
    if (pos + 1 >= *n) {
      size_t new_size = *n * 2;
      char *new_ptr = realloc(*lineptr, new_size);
      if (new_ptr == NULL) {
        return -1;
      }
      *lineptr = new_ptr;
      *n = new_size;
    }

    (*lineptr)[pos++] = (char)c;

    if (c == '\n') {
      break;
    }
  }

  if (pos == 0 && c == EOF) {
    return -1; // EOF and no data read
  }

  (*lineptr)[pos] = '\0';
  return (ssize_t)pos;
}
#endif

const ArErr no_err = (ArErr){false};

ArErr create_err(int64_t line, int64_t column, int length, char *path,
                 const char *type, const char *fmt, ...) {
  ArErr err;
  err.exists = true;
  err.path = path;
  err.line = line;
  err.column = column;
  err.length = length;

  // Copy error type safely
  snprintf(err.type, sizeof(err.type), "%s",(char*)type);

  // Format error message
  va_list args;
  va_start(args, fmt);
  vsnprintf(err.message, sizeof(err.message), fmt, args);
  va_end(args);

  return err;
}

void output_err(ArErr err) {
  if (!err.exists)
    return;
  dye(stderr, DYE_WHITE, DYE_RED);
  fprintf(stderr, "ERROR!");
  dye(stderr, DYE_RESET, DYE_RESET);
  fprintf(stderr, " ");
  dyefg(stderr, DYE_RED);
  dye_style(stderr, DYE_STYLE_BOLD);
  fprintf(stderr, "%s", err.type);
  dye_style(stderr, DYE_STYLE_RESET);
  dyefg(stderr, DYE_RESET);
  fprintf(stderr, ": ");
  dyefg(stderr, DYE_RED);
  fprintf(stderr, "%s", err.message);
  dye_style(stderr, DYE_STYLE_RESET);
  dyefg(stderr, DYE_RESET);
  fprintf(stderr, "\n");

  if (err.path && err.line) {
    dyefg(stderr, DYE_GRAY);
    fprintf(stderr, "  --> ");
    dyefg(stderr, DYE_CYAN);
    fprintf(stderr, "%s", err.path);
    dyefg(stderr, DYE_GRAY);
    fprintf(stderr, ":");
    dyefg(stderr, DYE_YELLOW);
    fprintf(stderr, "%" PRIu64 , err.line);
    dyefg(stderr, DYE_GRAY);
    fprintf(stderr, ":");
    dyefg(stderr, DYE_YELLOW);
    fprintf(stderr, "%" PRIu64, err.column);
    dye_style(stderr, DYE_STYLE_RESET);
    dyefg(stderr, DYE_RESET);
    fprintf(stderr, "\n");
    FILE *file = fopen(err.path, "r");
    if (file) {
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      int line_number_width = snprintf(NULL, 0, "%" PRIu64, err.line);
      char *buffer = NULL;
      size_t size = 0;
      int current_line = 1;
      ssize_t len;

      while ((len = getline(&buffer, &size, file)) != -1) {
        if (current_line == err.line) {
          break;
        }
        current_line++;
      }
      fprintf(stderr, "  ");
      for (int i = 0; i < line_number_width; i++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "|\n");
      for (ssize_t i = 0;i<len;i++) {
        if (buffer[i] == '\n') {
          buffer[i] = '\0';
          break;
        }
      }
      char *line_starts = buffer;
      size_t skipped_chars = 0;
      while (*line_starts && isspace((unsigned char)*line_starts) &&
             line_starts - buffer < err.column - 1) {
        line_starts++;
        err.column--;
        skipped_chars++;
      }
      fprintf(stderr, " %" PRIu64 " | ", err.line);
      if (err.length) {
        fprintf(stderr, "%.*s", (int)err.column - 1, line_starts);
        dyefg(stderr, DYE_RED);
        dye_style(stderr, DYE_STYLE_BOLD);
        fprintf(stderr, "%.*s", err.length, line_starts + err.column - 1);
        dye_style(stderr, DYE_STYLE_RESET);
        dyefg(stderr, DYE_RESET);
        fprintf(stderr, "%.*s",
                (int)len - (int)skipped_chars - (int)err.column -
                    (int)err.length+1,
                line_starts + (int)err.column + err.length - 1);
        for (int64_t i = 0; i < err.column - 1; i++) {
          fprintf(stderr, " ");
        }
      } else {
        fprintf(stderr, "%s", line_starts);
      }
      free(buffer);
      fprintf(stderr, "\n  ");
      for (int i = 0; i < line_number_width; i++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "| ");

      for (int i = 1; i < err.column; i++) {
        fprintf(stderr, " ");
      }
      dyefg(stderr, DYE_RED);
      dye_style(stderr, DYE_STYLE_BOLD);
      for (int i = 0; i < err.length; i++) {
        fprintf(stderr, "^");
      }
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      fprintf(stderr, "\n");
    }
  }
}