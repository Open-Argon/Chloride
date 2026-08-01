// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "./handle.h"
#include "Argon.h"
#include "ArgonFunction.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

ARGON_FUNCTION(open_handle, {
  if (api->fix_to_arg_size(6, argc, err))
    return api->ARGON_NULL;
  ArgonObject *handle_obj = api->create_argon_buffer(sizeof(FileHandle));
  struct buffer handle_buffer = api->argon_buffer_to_buffer(handle_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct string path_str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct string mode_str = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  char *path = malloc(path_str.length + 1);
  if (!path)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");
  memcpy(path, path_str.data, path_str.length);
  path[path_str.length] = '\0';

  char *mode = malloc(mode_str.length + 1);
  if (!mode) {
    free(path);
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");
  }
  memcpy(mode, mode_str.data, mode_str.length);
  mode[mode_str.length] = '\0';

  FileHandle *handle = handle_buffer.data;
  handle->fp = fopen(path, mode);
  handle->is_open = true;
  handle->type = FILE_NORMAL;

  if (!handle->fp) {

    int errnum = errno;
    switch (errnum) {
    case ENOENT:
      api->throw_argon_error(err, argv[2], "%s", path);
      break;
    case EACCES:
      api->throw_argon_error(err, argv[3], "%s", path);
      break;
    case EEXIST:
      api->throw_argon_error(err, argv[4], "%s", path);
      break;
    default:
      api->throw_argon_error(err, argv[5], "%s", strerror(errnum));
      break;
    }
    free(path);
    free(mode);
    return api->ARGON_NULL;
  }
  free(path);
  free(mode);
  return handle_obj;
})

ARGON_FUNCTION(read_all, {
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  bool is_binary = argv[1] == api->ARGON_TRUE;

  FileHandle *handle = handle_buffer.data;

  if (!handle->is_open)
    return api->throw_argon_error(err, argv[2], "file is closed");
  char *buf = NULL;
  size_t total = 0;

  // try seekable path first
  if (fseek(handle->fp, 0, SEEK_END) == 0) {
    long size = ftell(handle->fp);
    if (size >= 0 && fseek(handle->fp, 0, SEEK_SET) == 0) {
      // seekable, allocate exact size
      buf = malloc(size + 1);
      if (!buf)
        return api->throw_argon_error(err, argv[1], "out of memory");

      total = fread(buf, 1, size, handle->fp);
      if (ferror(handle->fp)) {
        free(buf);
        return api->throw_argon_error(err, argv[1], "%s", strerror(errno));
      }
      buf[total] = '\0';
      goto done;
    }
  }
  clearerr(handle->fp);
  char chunk[4096];
  size_t n;
  while ((n = fread(chunk, 1, sizeof(chunk), handle->fp)) > 0) {
    char *tmp = realloc(buf, total + n);
    if (!tmp) {
      free(buf);
      return api->throw_argon_error(err, argv[1], "out of memory");
    }
    buf = tmp;
    memcpy(buf + total, chunk, n);
    total += n;
  }

  if (ferror(handle->fp)) {
    free(buf);
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));
  }

done:;
  ArgonObject *result;
  if (is_binary) {
    result = api->create_argon_buffer(total);
    struct buffer result_buffer = api->argon_buffer_to_buffer(result, err);
    memcpy(result_buffer.data, buf, total);
  } else {
    result = api->string_to_argon((struct string){buf, total});
  }
  free(buf);
  return result;
})

ARGON_FUNCTION(read, {
  if (api->fix_to_arg_size(4, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  bool is_binary = argv[1] == api->ARGON_TRUE;

  // get size argument
  int64_t size = api->argon_to_i64(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  if (size < 0)
    return api->throw_argon_error(err, argv[3], "read size cannot be negative");

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->throw_argon_error(err, argv[3], "file is closed");

  char *buf = malloc(size);
  if (!buf)
    return api->throw_argon_error(err, argv[3], "out of memory");

  size_t n = fread(buf, 1, size, handle->fp);

  if (ferror(handle->fp)) {
    free(buf);
    return api->throw_argon_error(err, argv[3], "%s", strerror(errno));
  }

  // EOF with nothing read
  if (n == 0) {
    free(buf);
    if (is_binary) {
      return api->create_argon_buffer(0);
    }
    return api->ARGON_NULL;
  }

  ArgonObject *result;
  if (is_binary) {
    result = api->create_argon_buffer(n);
    struct buffer result_buffer = api->argon_buffer_to_buffer(result, err);
    memcpy(result_buffer.data, buf, n);
  } else {
    result = api->string_to_argon((struct string){buf, n});
  }
  free(buf);
  return result;
})

ARGON_FUNCTION(write, {
  if (api->fix_to_arg_size(4, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->throw_argon_error(err, argv[3], "file is closed");

  bool is_binary = argv[2] == api->ARGON_TRUE;

  const char *data;
  size_t length;

  if (is_binary) {
    struct buffer buf = api->argon_buffer_to_buffer(argv[1], err);
    if (api->is_error(err))
      return api->ARGON_NULL;
    data = buf.data;
    length = buf.size;
  } else {
    struct string str = api->argon_to_string(argv[1], err);
    if (api->is_error(err))
      return api->ARGON_NULL;
    data = str.data;
    length = str.length;
  }

  size_t written = fwrite(data, 1, length, handle->fp);

  if (written != length) {
    return api->throw_argon_error(err, argv[3], "%s", strerror(errno));
  }

  return api->i64_to_argon(written);
})

ARGON_FUNCTION(close, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->ARGON_NULL;

  if (handle->type == FILE_NORMAL && fclose(handle->fp) != 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  handle->fp = NULL;
  handle->is_open = false;
  return api->ARGON_NULL;
})

ARGON_FUNCTION(seek, {
  if (api->fix_to_arg_size(4, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->throw_argon_error(err, argv[3], "file is closed");

  int64_t offset = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  int64_t whence = api->argon_to_i64(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  // validate whence
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
    return api->throw_argon_error(err, argv[3], "invalid whence value: %lld",
                                  whence);

  if (fseek(handle->fp, (long)offset, (int)whence) != 0)
    return api->throw_argon_error(err, argv[3], "%s", strerror(errno));

  return api->ARGON_NULL;
})

ARGON_FUNCTION(tell, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->throw_argon_error(err, argv[1], "file is closed");

  long pos = ftell(handle->fp);
  if (pos < 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  return api->i64_to_argon((int64_t)pos);
})

ARGON_FUNCTION(flush, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->throw_argon_error(err, argv[1], "file is closed");

  if (fflush(handle->fp) != 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  return api->ARGON_NULL;
})

ARGON_FUNCTION(file_size, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer handle_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  if (!handle->is_open)
    return api->throw_argon_error(err, argv[1], "file is closed");

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
  struct stat st;
  if (fstat(fileno(handle->fp), &st) == 0)
    return api->i64_to_argon((int64_t)st.st_size);
  // fall through to seek/tell if fstat fails for some reason
#endif

  long current = ftell(handle->fp);
  if (current < 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  if (fseek(handle->fp, 0, SEEK_END) != 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  long size = ftell(handle->fp);
  if (size < 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  if (fseek(handle->fp, current, SEEK_SET) != 0)
    return api->throw_argon_error(err, argv[1], "%s", strerror(errno));

  return api->i64_to_argon((int64_t)size);
})

ARGON_FUNCTION(path_type, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct string path_str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  char *path = malloc(path_str.length + 1);
  if (!path)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");

  memcpy(path, path_str.data, path_str.length);
  path[path_str.length] = '\0';

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
  struct stat st;
  int result = 0;

  if (stat(path, &st) == 0) {
    if (S_ISREG(st.st_mode))
      result = 1;
    else if (S_ISDIR(st.st_mode))
      result = 2;
  }

  free(path);
  return api->i64_to_argon(result);
#else
  FILE *fp = fopen(path, "rb");
  free(path);

  if (fp) {
    fclose(fp);
    return api->i64_to_argon(1);
  }

  return api->i64_to_argon(0);
#endif
})

ARGON_FUNCTION(mkdir, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct string path_str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  char *path = malloc(path_str.length + 1);
  if (!path)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");

  memcpy(path, path_str.data, path_str.length);
  path[path_str.length] = '\0';

  bool result = false;

#if defined(_WIN32) || defined(_WIN64)

  if (CreateDirectoryA(path, NULL)) {
    result = true;
  } else {
    DWORD error = GetLastError();

    // Directory already exists
    if (error == ERROR_ALREADY_EXISTS)
      result = true;
  }

#else

  if (mkdir(path, 0755) == 0) {
    result = true;
  } else {
    // Directory already exists
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
      result = true;
  }

#endif

  free(path);
  return result?api->ARGON_TRUE:api->ARGON_FALSE;
})

ARGON_FUNCTION(mkdir_p, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct string path_str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  char *path = malloc(path_str.length + 1);
  if (!path)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");

  memcpy(path, path_str.data, path_str.length);
  path[path_str.length] = '\0';

  int result = 1;

  size_t len = strlen(path);

  // Remove trailing separators (but preserve roots like "/" and "C:\")
  while (len > 1 &&
         (path[len - 1] == '/' || path[len - 1] == '\\')) {
#if defined(_WIN32) || defined(_WIN64)
    if (len == 3 && path[1] == ':')
      break;
#endif
    path[len - 1] = '\0';
    len--;
  }

  char *start = path + 1;

#if defined(_WIN32) || defined(_WIN64)
  // Skip drive prefix (C:\)
  if (len >= 2 && path[1] == ':')
    start = path + 2;

  // Skip initial slash after drive
  if (*start == '/' || *start == '\\')
    start++;
#endif

  for (char *p = start; *p; p++) {
    if (*p == '/' || *p == '\\') {
      char old = *p;
      *p = '\0';

      if (strlen(path) > 0) {

#if defined(_WIN32) || defined(_WIN64)

        if (!CreateDirectoryA(path, NULL)) {
          DWORD error = GetLastError();

          if (error != ERROR_ALREADY_EXISTS) {
            result = 0;
            *p = old;
            break;
          }
        }

#else

        if (mkdir(path, 0755) != 0) {
          struct stat st;

          if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            result = 0;
            *p = old;
            break;
          }
        }

#endif

      }

      *p = old;
    }
  }

  // Create the final directory
  if (result) {

#if defined(_WIN32) || defined(_WIN64)

    if (!CreateDirectoryA(path, NULL)) {
      DWORD error = GetLastError();

      if (error != ERROR_ALREADY_EXISTS)
        result = 0;
    }

#else

    if (mkdir(path, 0755) != 0) {
      struct stat st;

      if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
        result = 0;
    }

#endif

  }

  free(path);

  return result ? api->ARGON_TRUE : api->ARGON_FALSE;
})

ARGON_FUNCTION(open_stdout, {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  ArgonObject *handle_obj = api->create_argon_buffer(sizeof(FileHandle));
  struct buffer handle_buffer = api->argon_buffer_to_buffer(handle_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  handle->fp = stdout;
  handle->is_open = true;
  handle->type = FILE_STD;

  return handle_obj;
})

ARGON_FUNCTION(open_stdin, {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  ArgonObject *handle_obj = api->create_argon_buffer(sizeof(FileHandle));
  struct buffer handle_buffer = api->argon_buffer_to_buffer(handle_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  handle->fp = stdin;
  handle->is_open = true;
  handle->type = FILE_STD;

  return handle_obj;
})

ARGON_FUNCTION(open_stderr, {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  ArgonObject *handle_obj = api->create_argon_buffer(sizeof(FileHandle));
  struct buffer handle_buffer = api->argon_buffer_to_buffer(handle_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  handle->fp = stderr;
  handle->is_open = true;
  handle->type = FILE_STD;

  return handle_obj;
})

ARGON_FUNCTION(open_stdnull, {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  ArgonObject *handle_obj = api->create_argon_buffer(sizeof(FileHandle));
  struct buffer handle_buffer = api->argon_buffer_to_buffer(handle_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  FileHandle *handle = handle_buffer.data;
  #ifdef _WIN32
    handle->fp = fopen("NUL", "w");
  #else
    handle->fp = fopen("/dev/null", "w");
  #endif
  handle->is_open = true;
  handle->type = FILE_NULL;

  return handle_obj;
})

INIT_ARGON_MODULE({
  (void)vm;
  (void)err;
  REGISTER_ARGON_FUNCTION(open_handle)
  REGISTER_ARGON_FUNCTION(read_all)
  REGISTER_ARGON_FUNCTION(read)
  REGISTER_ARGON_FUNCTION(write)
  REGISTER_ARGON_FUNCTION(close)
  REGISTER_ARGON_FUNCTION(seek)
  REGISTER_ARGON_FUNCTION(tell)
  REGISTER_ARGON_FUNCTION(flush)
  REGISTER_ARGON_FUNCTION(file_size)
  REGISTER_ARGON_FUNCTION(path_type)
  REGISTER_ARGON_FUNCTION(open_stdout)
  REGISTER_ARGON_FUNCTION(open_stdin)
  REGISTER_ARGON_FUNCTION(open_stderr)
  REGISTER_ARGON_FUNCTION(open_stdnull)
})