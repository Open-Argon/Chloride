// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"

#include "../../file/native/handle.h"

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
  pid_t pid;
  bool waited;
  int exit_code;
} ProcessHandle;

static char **argon_array_to_argv(ArgonNativeAPI *api, ArgonObject *array_obj,
                                  ArgonError *err) {
  struct array array = api->argon_to_array(array_obj, err);

  if (api->is_error(err))
    return NULL;

  char **argv = malloc(sizeof(char *) * (array.size + 1));

  if (!argv)
    return NULL;

  for (size_t i = 0; i < array.size; i++) {
    struct string str = api->argon_to_string(array.items[i], err);

    if (api->is_error(err)) {
      free(argv);
      return NULL;
    }

    argv[i] = malloc(str.length + 1);

    memcpy(argv[i], str.data, str.length);

    argv[i][str.length] = '\0';
  }

  argv[array.size] = NULL;

  return argv;
}

static void free_argv(char **argv) {
  if (!argv)
    return;

  for (size_t i = 0; argv[i]; i++)
    free(argv[i]);

  free(argv);
}

ARGON_FUNCTION(process_start, {
  if (api->fix_to_arg_size(4, argc, err))
    return api->ARGON_NULL;

  char **process_args = argon_array_to_argv(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  struct buffer stdin_buffer = api->argon_buffer_to_buffer(argv[1], err);

  struct buffer stdout_buffer = api->argon_buffer_to_buffer(argv[2], err);

  struct buffer stderr_buffer = api->argon_buffer_to_buffer(argv[3], err);

  FileHandle *stdin_handle = (FileHandle *)stdin_buffer.data;

  FileHandle *stdout_handle = (FileHandle *)stdout_buffer.data;

  FileHandle *stderr_handle = (FileHandle *)stderr_buffer.data;

  pid_t pid = fork();

  if (pid == 0) {
    if (stdin_handle->type != FILE_NULL) {
      dup2(fileno(stdin_handle->fp), STDIN_FILENO);
    }

    if (stdout_handle->type != FILE_NULL) {
      dup2(fileno(stdout_handle->fp), STDOUT_FILENO);
    }

    if (stderr_handle->type != FILE_NULL) {
      dup2(fileno(stderr_handle->fp), STDERR_FILENO);
    }

    execvp(process_args[0], process_args);

    exit(127);
  }

  free_argv(process_args);

  ArgonObject *process = api->create_argon_buffer(sizeof(ProcessHandle));

  struct buffer process_buffer = api->argon_buffer_to_buffer(process, err);

  ProcessHandle *handle = (ProcessHandle *)process_buffer.data;

  handle->pid = pid;
  handle->waited = false;
  handle->exit_code = -1;

  return process;
})

ARGON_FUNCTION(process_wait, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct buffer process_buffer = api->argon_buffer_to_buffer(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  ProcessHandle *handle = (ProcessHandle *)process_buffer.data;

  if (handle->waited) {
    return api->i64_to_argon(handle->exit_code);
  }

  int status;

  if (waitpid(handle->pid, &status, 0) == -1) {
    return api->ARGON_NULL;
  }

  handle->waited = true;

  if (WIFEXITED(status)) {
    handle->exit_code = WEXITSTATUS(status);
  } else {
    handle->exit_code = -1;
  }

  return api->i64_to_argon(handle->exit_code);
})

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;
  REGISTER_ARGON_FUNCTION(process_start)
  REGISTER_ARGON_FUNCTION(process_wait)
}