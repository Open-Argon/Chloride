/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "err.h"
#include "../external/libdye/include/dye.h"
#include "memory.h"
#include "runtime/internals/dynamic_array_armem/darray_armem.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if defined(_WIN32) || defined(_WIN64)
#include "getline.h"
#endif

const ArErr no_err = (ArErr){"", "", false, NULL};

ArErr create_err(const char *type, const char *fmt, ...) {
  ArErr err;
  err.exists = true;

  err.stack_trace = darray_armem_create();

  darray_armem_init(err.stack_trace, sizeof(struct StackTraceFrame), 0);

  snprintf(err.type, sizeof(err.type), "%s", (char *)type);

  va_list args;
  va_start(args, fmt);
  vsnprintf(err.message, sizeof(err.message), fmt, args);
  va_end(args);

  return err;
}
ArErr path_specific_create_err(int64_t line, int64_t column, int64_t length,
                               char *path, const char *type, const char *fmt,
                               ...) {
  ArErr err;
  err.exists = true;

  err.stack_trace = darray_armem_create();

  darray_armem_init(err.stack_trace, sizeof(struct StackTraceFrame), 0);

  struct StackTraceFrame frame = {line, column, length, path};
  darray_armem_insert(err.stack_trace, err.stack_trace->size, &frame);

  snprintf(err.type, sizeof(err.type), "%s", (char *)type);

  va_list args;
  va_start(args, fmt);
  vsnprintf(err.message, sizeof(err.message), fmt, args);
  va_end(args);

  return err;
}

ArErr vcreate_err(const char *type, const char *fmt, va_list args) {
  ArErr err;
  err.exists = true;

  err.stack_trace = darray_armem_create();

  darray_armem_init(err.stack_trace, sizeof(struct StackTraceFrame), 0);

  snprintf(err.type, sizeof(err.type), "%s", type);
  vsnprintf(err.message, sizeof(err.message), fmt, args);
  return err;
}

void output_err(ArErr *err) {
  if (!err->exists)
    return;
  if (err->stack_trace->size > 1) {
    dyefg(stderr, DYE_RED);
    dye_style(stderr, DYE_STYLE_BOLD);
    fprintf(stderr, "Stack trace (oldest frame first):");
    dye_style(stderr, DYE_STYLE_RESET);
    dyefg(stderr, DYE_RESET);
    fprintf(stderr, "\n");
    for (int64_t i = err->stack_trace->size - 1; i >= 0; i--) {
      struct StackTraceFrame *frame = darray_armem_get(err->stack_trace, i);
      fprintf(stderr, " at ");
      dyefg(stderr, DYE_CYAN);
      if (frame->path)
        fprintf(stderr, "%s", frame->path);
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, ":");
      dyefg(stderr, DYE_YELLOW);
      fprintf(stderr, "%" PRIu64, frame->line);
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, ":");
      dyefg(stderr, DYE_YELLOW);
      fprintf(stderr, "%" PRIu64, frame->column);
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
  }
  dye(stderr, DYE_WHITE, DYE_RED);
  fprintf(stderr, "ERROR!");
  dye(stderr, DYE_RESET, DYE_RESET);
  fprintf(stderr, " ");
  dyefg(stderr, DYE_RED);
  dye_style(stderr, DYE_STYLE_BOLD);
  fprintf(stderr, "%s", err->type);
  dye_style(stderr, DYE_STYLE_RESET);
  dyefg(stderr, DYE_RESET);
  fprintf(stderr, ": ");
  dyefg(stderr, DYE_RED);
  fprintf(stderr, "%s", err->message);
  dye_style(stderr, DYE_STYLE_RESET);
  dyefg(stderr, DYE_RESET);
  fprintf(stderr, "\n");

  if (err->stack_trace->size > 0) {
    struct StackTraceFrame *frame =
        darray_armem_get(err->stack_trace, 0);
    dyefg(stderr, DYE_GRAY);
    fprintf(stderr, "  --> ");
    dyefg(stderr, DYE_CYAN);
    fprintf(stderr, "%s", frame->path);
    dyefg(stderr, DYE_GRAY);
    fprintf(stderr, ":");
    dyefg(stderr, DYE_YELLOW);
    fprintf(stderr, "%" PRIu64, frame->line);
    dyefg(stderr, DYE_GRAY);
    fprintf(stderr, ":");
    dyefg(stderr, DYE_YELLOW);
    fprintf(stderr, "%" PRIu64, frame->column);
    dye_style(stderr, DYE_STYLE_RESET);
    dyefg(stderr, DYE_RESET);
    fprintf(stderr, "\n");
    FILE *file = fopen(frame->path, "r");
    if (file) {
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      int line_number_width = snprintf(NULL, 0, "%" PRIu64, frame->line);
      char *buffer = NULL;
      size_t size = 0;
      uint64_t current_line = 1;
      ssize_t len;

      while ((len = getline(&buffer, &size, file)) != -1) {
        if (current_line == frame->line) {
          break;
        }
        current_line++;
      }
      fprintf(stderr, "  ");
      for (int i = 0; i < line_number_width; i++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "|\n");
      for (ssize_t i = 0; i < len; i++) {
        if (buffer[i] == '\n') {
          buffer[i] = '\0';
          break;
        }
      }
      char *line_starts = buffer;
      size_t skipped_chars = 0;
      while (*line_starts && isspace((unsigned char)*line_starts) &&
             line_starts - buffer < (int64_t)frame->column - 1) {
        line_starts++;
        frame->column--;
        skipped_chars++;
      }
      fprintf(stderr, " %" PRIu64 " | ", frame->line);
      if (frame->length) {
        fprintf(stderr, "%.*s", (int)frame->column - 1, line_starts);
        dyefg(stderr, DYE_RED);
        dye_style(stderr, DYE_STYLE_BOLD);
        fprintf(stderr, "%.*s", (int)frame->length,
                line_starts + frame->column - 1);
        dye_style(stderr, DYE_STYLE_RESET);
        dyefg(stderr, DYE_RESET);
        fprintf(stderr, "%.*s",
                (int)len - (int)skipped_chars - (int)frame->column -
                    (int)frame->length + 1,
                line_starts + (int)frame->column + frame->length - 1);
        for (uint64_t i = 0; i < frame->column - 1; i++) {
          fprintf(stderr, " ");
        }
      } else {
        fprintf(stderr, "%s", line_starts);
      }
      free(buffer);
      fprintf(stderr, "\n  ");
      for (uint64_t i = 0; i < line_number_width; i++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "| ");

      for (uint64_t i = 1; i < frame->column; i++) {
        fprintf(stderr, " ");
      }
      dyefg(stderr, DYE_RED);
      dye_style(stderr, DYE_STYLE_BOLD);
      for (uint64_t i = 0; i < frame->length; i++) {
        fprintf(stderr, "^");
      }
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      fprintf(stderr, "\n");
    }
  }
}