/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "err.h"
#include "../external/libdye/include/dye.h"
#include "arobject.h"
#include "memory.h"
#include "runtime/api/api.h"
#include "runtime/internals/dynamic_array_armem/darray_armem.h"
#include "runtime/objects/array/array.h"
#include "runtime/objects/literals/literals.h"
#include "runtime/objects/number/number.h"
#include "runtime/objects/object.h"
#include "runtime/objects/string/string.h"
#include "runtime/objects/tuple/tuple.h"
#include "runtime/runtime.h"
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

ArErr no_err = {NULL};

bool is_error(ArErr *err) { return err->ptr != ARGON_NULL; }

char *vfmt_alloc(const char *fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);

  int len = vsnprintf(NULL, 0, fmt, args);
  if (len < 0) {
    va_end(args_copy);
    return NULL;
  }

  char *buf = malloc(len + 1);
  if (!buf) {
    va_end(args_copy);
    return NULL;
  }

  vsnprintf(buf, len + 1, fmt, args_copy);
  va_end(args_copy);

  return buf;
}

ArErr create_err(ArgonObject *type, const char *fmt, ...) {
  ArgonObject *err_instance = new_instance(type, 0);

  va_list args;
  va_start(args, fmt);
  char *msg = vfmt_alloc(fmt, args);
  va_end(args);

  add_builtin_field(err_instance, message,
                    new_string_object_null_terminated(msg));

  free(msg);

  add_builtin_field(err_instance, stack_trace,
                    ARRAY_CREATE(0, NULL, NULL, NULL, NULL));

  return (ArErr){.ptr = err_instance};
}

ArErr path_specific_create_err(int64_t line, int64_t column, int64_t length,
                               char *path, ArgonObject *type, const char *fmt,
                               ...) {
  ArgonObject *err_instance = new_instance(type, 0);

  va_list args;
  va_start(args, fmt);
  char *msg = vfmt_alloc(fmt, args);
  va_end(args);

  add_builtin_field(err_instance, message,
                    new_string_object_null_terminated(msg));

  free(msg);

  add_builtin_field(
      err_instance, stack_trace,
      ARRAY_CREATE(
          1,
          (ArgonObject *[]){TUPLE_CREATE(
              4,
              (ArgonObject *[]){new_string_object_null_terminated(path),
                                new_number_object_from_int64(line),
                                new_number_object_from_int64(column),
                                new_number_object_from_int64(length)},
              NULL, NULL, NULL)},
          NULL, NULL, NULL));

  return (ArErr){.ptr = err_instance};
}

ArErr vcreate_err(ArgonObject *type, const char *fmt, va_list args) {
  ArgonObject *err_instance = new_instance(type, 0);

  char *msg = vfmt_alloc(fmt, args);

  add_builtin_field(err_instance, message,
                    new_string_object_null_terminated(msg));

  free(msg);

  add_builtin_field(err_instance, stack_trace,
                    ARRAY_CREATE(0, NULL, NULL, NULL, NULL));

  return (ArErr){.ptr = err_instance};
}

struct StackTraceFrame {
  char *path;
  int64_t line;
  int64_t column;
  int64_t length;
};

bool convert_to_stackTraceFrame(ArgonObject *frame_obj,
                                struct StackTraceFrame *frame) {
  if (!frame_obj || frame_obj->type != TYPE_TUPLE ||
      frame_obj->value.as_tuple.size != 4)
    return false;
  ArgonObject **tuple = frame_obj->value.as_tuple.data;
  if (tuple[0]->type != TYPE_STRING || tuple[1]->type != TYPE_NUMBER ||
      !tuple[1]->value.as_number->is_int64 || tuple[2]->type != TYPE_NUMBER ||
      !tuple[2]->value.as_number->is_int64 || tuple[3]->type != TYPE_NUMBER ||
      !tuple[3]->value.as_number->is_int64)
    return false;
  frame->path = argon_string_to_c_string_malloc(tuple[0]);
  frame->line = tuple[1]->value.as_number->n.i64;
  frame->column = tuple[2]->value.as_number->n.i64;
  frame->length = tuple[3]->value.as_number->n.i64;
  return true;
}

void output_err(ArErr *err) {
  Translated gc_translated = {UINT8_MAX,         0,  0,  {-1, 0},  {NULL, 0},
                              {NULL, 0}, {}, {}, "<error>"};
  RuntimeState state;
  init_runtime_state(&state, gc_translated, "<error>");
  if (!is_error(err))
    return;
  ArgonObject *stack_trace_obj = get_builtin_field(err->ptr, stack_trace);
  darray_armem *stack_trace = NULL;
  if (stack_trace_obj && stack_trace_obj->type == TYPE_ARRAY)
    stack_trace = stack_trace_obj->value.as_array;
  if (stack_trace_obj && stack_trace && stack_trace->size > 1) {
    dyefg(stderr, DYE_RED);
    dye_style(stderr, DYE_STYLE_BOLD);
    fprintf(stderr, "Stack trace (oldest frame first):");
    dye_style(stderr, DYE_STYLE_RESET);
    dyefg(stderr, DYE_RESET);
    fprintf(stderr, "\n");
    for (int64_t i = stack_trace->size - 1; i >= 0; i--) {
      ArgonObject **frame_obj = darray_armem_get(stack_trace, i);
      struct StackTraceFrame frame;
      if (!convert_to_stackTraceFrame(*frame_obj, &frame))
        continue;
      fprintf(stderr, " at ");
      dyefg(stderr, DYE_CYAN);
      if (frame.path)
        fprintf(stderr, "%s", frame.path);
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, ":");
      dyefg(stderr, DYE_YELLOW);
      fprintf(stderr, "%" PRIu64, frame.line);
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, ":");
      dyefg(stderr, DYE_YELLOW);
      fprintf(stderr, "%" PRIu64, frame.column);
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      fprintf(stderr, "\n");
      free(frame.path);
    }
    fprintf(stderr, "\n");
  }
  dye(stderr, DYE_WHITE, DYE_RED);
  fprintf(stderr, "ERROR!");
  dye(stderr, DYE_RESET, DYE_RESET);
  fprintf(stderr, " ");
  dyefg(stderr, DYE_RED);
  dye_style(stderr, DYE_STYLE_BOLD);
  ArgonObject *type_name_obj = get_builtin_field_for_class(
      get_builtin_field(err->ptr, __class__), __name__, err->ptr);
  if (type_name_obj) {
    ArErr err = {ARGON_NULL};
    size_t length;
    char *msg = argon_object_to_length_terminated_string_from___string__(
        type_name_obj, &err, &state, &length);
    fwrite(msg, sizeof(char), length, stderr);
  } else {
    fprintf(stderr, "UnknownException");
  }
  dye_style(stderr, DYE_STYLE_RESET);
  dyefg(stderr, DYE_RESET);
  fprintf(stderr, ": ");
  dyefg(stderr, DYE_RED);
  ArgonObject *message_obj = get_builtin_field(err->ptr, message);
  if (message_obj) {
    ArErr err = {ARGON_NULL};
    size_t length;
    char *msg = argon_object_to_length_terminated_string_from___string__(
        message_obj, &err, &state, &length);
    fwrite(msg, sizeof(char), length, stderr);
  }
  dye_style(stderr, DYE_STYLE_RESET);
  dyefg(stderr, DYE_RESET);
  fprintf(stderr, "\n");

  if (stack_trace->size > 0) {
    ArgonObject **frame_obj = darray_armem_get(stack_trace, 0);
    struct StackTraceFrame frame;
    if (convert_to_stackTraceFrame(*frame_obj, &frame)) {
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, "  --> ");
      dyefg(stderr, DYE_CYAN);
      fprintf(stderr, "%s", frame.path);
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, ":");
      dyefg(stderr, DYE_YELLOW);
      fprintf(stderr, "%" PRIu64, frame.line);
      dyefg(stderr, DYE_GRAY);
      fprintf(stderr, ":");
      dyefg(stderr, DYE_YELLOW);
      fprintf(stderr, "%" PRIu64, frame.column);
      dye_style(stderr, DYE_STYLE_RESET);
      dyefg(stderr, DYE_RESET);
      fprintf(stderr, "\n");
      FILE *file = fopen(frame.path, "r");
      if (file) {
        dye_style(stderr, DYE_STYLE_RESET);
        dyefg(stderr, DYE_RESET);
        int64_t line_number_width = snprintf(NULL, 0, "%" PRIu64, frame.line);
        char *buffer = NULL;
        size_t size = 0;
        int64_t current_line = 1;
        ssize_t len;

        while ((len = getline(&buffer, &size, file)) != -1) {
          if (current_line == frame.line) {
            break;
          }
          current_line++;
        }
        fprintf(stderr, "  ");
        for (int64_t i = 0; i < line_number_width; i++) {
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
               line_starts - buffer < (int64_t)frame.column - 1) {
          line_starts++;
          frame.column--;
          skipped_chars++;
        }
        fprintf(stderr, " %" PRIu64 " | ", frame.line);
        if (frame.length) {
          fprintf(stderr, "%.*s", (int)frame.column - 1, line_starts);
          dyefg(stderr, DYE_RED);
          dye_style(stderr, DYE_STYLE_BOLD);
          fprintf(stderr, "%.*s", (int)frame.length,
                  line_starts + frame.column - 1);
          dye_style(stderr, DYE_STYLE_RESET);
          dyefg(stderr, DYE_RESET);
          fprintf(stderr, "%.*s",
                  (int)len - (int)skipped_chars - (int)frame.column -
                      (int)frame.length + 1,
                  line_starts + (int)frame.column + frame.length - 1);
          for (int64_t i = 0; i < frame.column - 1; i++) {
            fprintf(stderr, " ");
          }
        } else {
          fprintf(stderr, "%s", line_starts);
        }
        free(buffer);
        fprintf(stderr, "\n  ");
        for (int64_t i = 0; i < line_number_width; i++) {
          fprintf(stderr, " ");
        }
        fprintf(stderr, "| ");

        for (int64_t i = 1; i < frame.column; i++) {
          fprintf(stderr, " ");
        }
        dyefg(stderr, DYE_RED);
        dye_style(stderr, DYE_STYLE_BOLD);
        for (int64_t i = 0; i < frame.length; i++) {
          fprintf(stderr, "^");
        }
        dye_style(stderr, DYE_STYLE_RESET);
        dyefg(stderr, DYE_RESET);
        fprintf(stderr, "\n");
      }
      free(frame.path);
    }
  }
}