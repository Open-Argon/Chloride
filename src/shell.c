/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "./lexer/lexer.h"
#include "./runtime/call/call.h"
#include "./runtime/objects/functions/functions.h"
#include "./runtime/objects/term/term.h"
#include "./runtime/runtime.h"
#include "./translator/translator.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(__linux__)
#include <malloc.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
FILE *fmemopen(void *buf, size_t size, const char *mode) {
    FILE *fp = tmpfile();
    if (!fp) return NULL;

    if (strchr(mode, 'w') || strchr(mode, '+')) {
        fwrite(buf, 1, size, fp);
        rewind(fp);
    }
    return fp;
}
#else
#include "../external/linenoise/linenoise.h"
#endif

volatile sig_atomic_t interrupted = 0;

// Ctrl+C handler
void handle_sigint(int sig) {
  (void)sig;
  interrupted = 1;
}

int execute_code(FILE *stream, char *path, Stack *scope,
                 RuntimeState *runtime_state) {
  if (!stream) {
    perror("fmemopen");
    return 1;
  }

  ArErr err;

  DArray tokens;
  darray_init(&tokens, sizeof(Token));
  LexerState state = {path, stream, 0, 0, &tokens};
  err = lexer(state);
  if (err.exists) {
    darray_free(&tokens, free_token);
    output_err(err);
    return 1;
  }

  DArray ast;

  darray_init(&ast, sizeof(ParsedValue));

  err = parser(path, &ast, &tokens, false);
  darray_free(&tokens, free_token);
  if (err.exists) {
    darray_free(&ast, free_parsed);
    output_err(err);
    return 1;
  }
  Translated __translated = init_translator(path);
  err = translate(&__translated, &ast);
  darray_free(&ast, free_parsed);
  if (err.exists) {
    darray_free(&__translated.bytecode, NULL);
    free(__translated.constants.data);
    hashmap_free(__translated.constants.hashmap, NULL);
    output_err(err);
    return 1;
  }

  hashmap_free(__translated.constants.hashmap, NULL);
  Translated translated = {
      __translated.registerCount, NULL, {}, {}, __translated.path};
  translated.bytecode.data = ar_alloc(__translated.bytecode.capacity);
  memcpy(translated.bytecode.data, __translated.bytecode.data,
         __translated.bytecode.capacity);
  translated.bytecode.element_size = __translated.bytecode.element_size;
  translated.bytecode.size = __translated.bytecode.size;
  translated.bytecode.resizable = false;
  translated.bytecode.capacity =
      __translated.bytecode.size * __translated.bytecode.element_size;
  translated.constants.data = ar_alloc(__translated.constants.capacity);
  memcpy(translated.constants.data, __translated.constants.data,
         __translated.constants.capacity);
  translated.constants.size = __translated.constants.size;
  translated.constants.capacity = __translated.constants.capacity;
  darray_free(&__translated.bytecode, NULL);
  free(__translated.constants.data);
  *runtime_state = init_runtime_state(translated, path);
  err = runtime(translated, *runtime_state, scope);
  if (err.exists) {
    output_err(err);
    return 1;
  }
  return 0;
}

#if defined(_WIN32) || defined(_WIN64)
// Simple input function
char *input(const char *prompt) {
  printf("%s", prompt);
  fflush(stdout);

  char *buffer = NULL;
  size_t size = 0;
  ssize_t len = getline(&buffer, &size, stdin);

  if (len == -1) {
    free(buffer);
    return NULL;
  }

  // Remove trailing newline
  if (len > 0 && buffer[len - 1] == '\n') {
    buffer[len - 1] = '\0';
  }

  return buffer;
}
#endif

char *read_all_stdin(size_t *out_len) {
  size_t size = 1024;
  size_t len = 0;
  char *buffer = malloc(size);
  if (!buffer)
    exit(1);

  int c;
  while ((c = fgetc(stdin)) != EOF) {
    if (len + 1 >= size) {
      size *= 2;
      buffer = realloc(buffer, size);
      if (!buffer)
        exit(1);
    }
    buffer[len++] = (char)c;
  }

  *out_len = len;
  return buffer;
}

int shell() {

  Stack *main_scope = create_scope(Global_Scope);

  if (!isatty(STDIN_FILENO)) {
    RuntimeState runtime_state;
    size_t len;
    char *data = read_all_stdin(&len);
    FILE *file = fmemopen(data, len, "r");
    int resp = execute_code(file, "<stdin>", main_scope, &runtime_state);
    fclose(file);
    free(data);
    return resp;
  }

  signal(SIGINT, handle_sigint);

  printf("Welcome to the Argon shell!\n\n");

  ArgonObject *output_object = create_argon_native_function("log", term_log);
  char *totranslate = NULL;
  size_t totranslatelength = 0;

  while (true) {
#if defined(__linux__)
    malloc_trim(0);
#endif
    if (totranslate) {
      free(totranslate);
      totranslate = NULL;
      totranslatelength = 0;
    };
    int indent = 0;
    char textBefore[] = ">>> ";

    // Dynamic array of lines

    do {
      // indent string
      size_t isz = (size_t)indent * 4;
      char *indentStr = (char *)malloc(isz + 1);
      if (!indentStr)
        exit(1);
      memset(indentStr, ' ', isz);
      indentStr[isz] = '\0';

      // prompt
      size_t p_len = strlen(textBefore) + isz;
      char *prompt = (char *)malloc(p_len + 1);
      if (!prompt)
        exit(1);
      memcpy(prompt, textBefore, strlen(textBefore));
      memcpy(prompt + strlen(textBefore), indentStr, isz + 1);

#if defined(_WIN32) || defined(_WIN64)
      char *inp = input(prompt);
#else
      char *inp = linenoise(prompt);
#endif
      free(prompt);

      if (!inp) {
        printf("\nBye :)\n");
        // Free previously collected lines
        free(inp);
        free(totranslate);
        free(indentStr);
        return 0;
      }
#if defined(_WIN32) || defined(_WIN64)
#else
      if (inp[0] != '\0') {
        // Optionally add line to history
        linenoiseHistoryAdd(inp);
      }
#endif

      // Append line to totranslate
      size_t length = strlen(inp);
      totranslate = realloc(totranslate, totranslatelength + isz + length + 1);
      memcpy(totranslate + totranslatelength, indentStr, isz);
      memcpy(totranslate + totranslatelength + isz, inp, length);
      totranslatelength += isz + length + 1;
      totranslate[totranslatelength - 1] = '\n';

      char *trimmed = inp;
      while (*trimmed == ' ' || *trimmed == '\t')
        trimmed++;

      size_t len = strlen(trimmed);
      if (len >= 2 && strcmp(trimmed + len - 2, "do") == 0) {
        indent++;
      } else if (len == 0 && indent > 0) {
        indent--;
      }
      free(inp);
      strcpy(textBefore, "... ");
      free(indentStr);

    } while (indent > 0);
    totranslate = realloc(totranslate, totranslatelength + 1);
    totranslate[totranslatelength] = '\0';
    RuntimeState runtime_state;
    FILE *file = fmemopen((void *)totranslate, totranslatelength, "r");
    int resp = execute_code(file, "<shell>", main_scope, &runtime_state);
    fclose(file);
    if (resp) {
      continue;
    }
    ArErr err;
    argon_call(output_object, 1, (ArgonObject *[]){runtime_state.registers[0]},
               &err, &runtime_state);
    totranslatelength = 0;
  }

  return 0;
}