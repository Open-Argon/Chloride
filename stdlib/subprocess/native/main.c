// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"

#include "../../file/native/handle.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

typedef struct {
#ifdef _WIN32
  HANDLE process;
#else
  pid_t pid;
#endif

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
  if (api->fix_to_arg_size(5, argc, err))
    return api->ARGON_NULL;

  char **process_args = argon_array_to_argv(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  struct buffer stdin_buffer = api->argon_buffer_to_buffer(argv[1], err);
  struct buffer stdout_buffer = api->argon_buffer_to_buffer(argv[2], err);
  struct buffer stderr_buffer = api->argon_buffer_to_buffer(argv[3], err);
  char* cwd = NULL;
  if (argv[4] != api->ARGON_NULL) {
    cwd = api->argon_to_string(argv[4], err).data;
    if (api->is_error(err)) return api->ARGON_NULL;
  }

  FileHandle *stdin_handle = (FileHandle *)stdin_buffer.data;
  FileHandle *stdout_handle = (FileHandle *)stdout_buffer.data;
  FileHandle *stderr_handle = (FileHandle *)stderr_buffer.data;

  ArgonObject *process = api->create_argon_buffer(sizeof(ProcessHandle));

  struct buffer process_buffer = api->argon_buffer_to_buffer(process, err);

  ProcessHandle *handle = (ProcessHandle *)process_buffer.data;

#ifdef _WIN32

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  memset(&si, 0, sizeof(si));
  memset(&pi, 0, sizeof(pi));

  si.cb = sizeof(si);

  si.dwFlags |= STARTF_USESTDHANDLES;

  // A FILE_NULL handle (file.stdnull) connects the stream to the NUL device.
  SECURITY_ATTRIBUTES null_attributes = {sizeof(SECURITY_ATTRIBUTES), NULL,
                                         TRUE};
  HANDLE null_device = INVALID_HANDLE_VALUE;
  if (stdin_handle->type == FILE_NULL || stdout_handle->type == FILE_NULL ||
      stderr_handle->type == FILE_NULL)
    null_device = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              &null_attributes, OPEN_EXISTING, 0, NULL);

  if (stdin_handle->type != FILE_NULL)
    fflush(stdin_handle->fp);
  if (stdout_handle->type != FILE_NULL)
    fflush(stdout_handle->fp);
  if (stderr_handle->type != FILE_NULL)
    fflush(stderr_handle->fp);

  si.hStdInput = stdin_handle->type != FILE_NULL
                     ? (HANDLE)_get_osfhandle(fileno(stdin_handle->fp))
                     : null_device;

  si.hStdOutput = stdout_handle->type != FILE_NULL
                      ? (HANDLE)_get_osfhandle(fileno(stdout_handle->fp))
                      : null_device;

  si.hStdError = stderr_handle->type != FILE_NULL
                     ? (HANDLE)_get_osfhandle(fileno(stderr_handle->fp))
                     : null_device;

  // Windows wants a single command line, not argv[]
  size_t cmd_len = 0;

  for (size_t i = 0; process_args[i]; i++)
    cmd_len += strlen(process_args[i]) + 3;

  char *cmdline = malloc(cmd_len + 1);

  cmdline[0] = '\0';

  for (size_t i = 0; process_args[i]; i++) {
    strcat(cmdline, "\"");
    strcat(cmdline, process_args[i]);
    strcat(cmdline, "\" ");
  }

  BOOL ok =
      CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, cwd, &si, &pi);

  free(cmdline);

  if (null_device != INVALID_HANDLE_VALUE)
    CloseHandle(null_device);

  if (!ok) {
    free_argv(process_args);
    return api->ARGON_NULL;
  }

  CloseHandle(pi.hThread);

  handle->process = pi.hProcess;

#else

  // Flush anything buffered so it isn't written after the child's output.
  FileHandle *handles[] = {stdin_handle, stdout_handle, stderr_handle};
  for (size_t i = 0; i < 3; i++)
    if (handles[i]->type != FILE_NULL && handles[i]->fp)
      fflush(handles[i]->fp);

  pid_t pid = fork();

  if (pid == 0) {
    if (cwd && chdir(cwd) == -1)
        _exit(127);

    // A FILE_NULL handle (file.stdnull) connects the stream to /dev/null.
    int targets[] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    for (size_t i = 0; i < 3; i++) {
      if (handles[i]->type == FILE_NULL) {
        int null_fd = open("/dev/null", O_RDWR);
        if (null_fd >= 0) {
          dup2(null_fd, targets[i]);
          if (null_fd != targets[i])
            close(null_fd);
        }
      } else {
        dup2(fileno(handles[i]->fp), targets[i]);
      }
    }

    execvp(process_args[0], process_args);

    exit(127);
  }

  handle->pid = pid;

#endif

  free_argv(process_args);

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

  if (handle->waited)
    return api->i64_to_argon(handle->exit_code);

#ifdef _WIN32

  WaitForSingleObject(handle->process, INFINITE);

  DWORD code;

  if (GetExitCodeProcess(handle->process, &code)) {
    handle->exit_code = (int)code;
  } else {
    handle->exit_code = -1;
  }

  CloseHandle(handle->process);

#else

  int status;

  if (waitpid(handle->pid, &status, 0) == -1)
    return api->ARGON_NULL;

  if (WIFEXITED(status))
    handle->exit_code = WEXITSTATUS(status);

#endif

  handle->waited = true;

  return api->i64_to_argon(handle->exit_code);
})

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;
  REGISTER_ARGON_FUNCTION(process_start)
  REGISTER_ARGON_FUNCTION(process_wait)
}