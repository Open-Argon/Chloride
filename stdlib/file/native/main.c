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
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
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

#ifdef _WIN32

  struct _stat st;
  int result = 0;

  if (_stat(path, &st) == 0) {
    if ((st.st_mode & _S_IFMT) == _S_IFREG)
      result = 1;
    else if ((st.st_mode & _S_IFMT) == _S_IFDIR)
      result = 2;
  }

  free(path);
  return api->i64_to_argon(result);

#else

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
  return result ? api->ARGON_TRUE : api->ARGON_FALSE;
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
  while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
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

/*
 * Temporary files/directories
 *
 * temp_file(pattern)
 * temp_dir(pattern)
 *
 * The pattern is relative to the platform's temporary directory unless
 * it contains an absolute path.
 *
 * '*' is replaced with random characters.
 *
 * Examples:
 *
 *   temp_file("argon-*")
 *   temp_file("argon-*.tmp")
 *   temp_dir("argon-*")
 *
 * The returned value is the actual path that was created.
 */

#define ARGON_TEMP_RANDOM_CHARS \
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

#define ARGON_TEMP_RANDOM_LENGTH 12

static const char *argon_get_temp_directory(void) {
#if defined(_WIN32) || defined(_WIN64)

  static char temp_path[MAX_PATH + 1];
  static int initialized = 0;

  if (!initialized) {
    DWORD length = GetTempPathA(MAX_PATH, temp_path);

    if (length == 0 || length > MAX_PATH)
      return NULL;

    /*
     * GetTempPathA normally returns a trailing backslash.
     * Keep it because Windows accepts it naturally.
     */
    temp_path[length] = '\0';
    initialized = 1;
  }

  return temp_path;

#else

  const char *temp_dir = getenv("TMPDIR");

  if (!temp_dir || !*temp_dir)
    temp_dir = getenv("TMP");

  if (!temp_dir || !*temp_dir)
    temp_dir = "/tmp";

  return temp_dir;

#endif
}

static bool argon_path_is_absolute(const char *path) {
#if defined(_WIN32) || defined(_WIN64)

  /*
   * C:\foo
   * C:/foo
   * \\server\share\foo
   * /foo
   */
  if (path[0] == '/' || path[0] == '\\')
    return true;

  if (((path[0] >= 'A' && path[0] <= 'Z') ||
       (path[0] >= 'a' && path[0] <= 'z')) &&
      path[1] == ':')
    return true;

  return false;

#else

  return path[0] == '/';

#endif
}

static char argon_temp_random_char(void) {
  return ARGON_TEMP_RANDOM_CHARS[
      (unsigned int)rand() %
      (sizeof(ARGON_TEMP_RANDOM_CHARS) - 1)];
}

static void argon_temp_fill_random(char *data, size_t length) {
  for (size_t i = 0; i < length; i++)
    data[i] = argon_temp_random_char();
}

/*
 * Replace the first '*' in pattern with random characters.
 *
 * If there is no '*', append random characters to the name. This keeps
 * the function useful while still guaranteeing a unique temporary name.
 */
static char *argon_temp_make_candidate(const char *pattern) {
  const char *star = strchr(pattern, '*');

  /*
   * No wildcard: use the supplied name exactly.
   *
   * This means:
   *
   *   temp_file("hello.txt")
   *
   * creates /tmp/hello.txt and fails if it already exists.
   */
  if (!star) {
    size_t length = strlen(pattern);

    char *result = malloc(length + 1);
    if (!result)
      return NULL;

    memcpy(result, pattern, length + 1);
    return result;
  }

  /*
   * Replace the first '*' with random characters.
   *
   *   hello-*.tar.gz
   *
   * becomes something like:
   *
   *   hello-a8F3kP91xQ2m.tar.gz
   */
  size_t prefix_len = (size_t)(star - pattern);
  size_t suffix_len = strlen(star + 1);

  size_t length =
      prefix_len +
      ARGON_TEMP_RANDOM_LENGTH +
      suffix_len;

  char *result = malloc(length + 1);

  if (!result)
    return NULL;

  memcpy(result, pattern, prefix_len);

  argon_temp_fill_random(
      result + prefix_len,
      ARGON_TEMP_RANDOM_LENGTH);

  memcpy(
      result + prefix_len + ARGON_TEMP_RANDOM_LENGTH,
      star + 1,
      suffix_len);

  result[length] = '\0';

  return result;
}

static char *argon_temp_join_path(
    const char *directory,
    const char *pattern) {

  if (argon_path_is_absolute(pattern)) {
    size_t length = strlen(pattern);

    char *result = malloc(length + 1);

    if (!result)
      return NULL;

    memcpy(result, pattern, length + 1);

    return result;
  }

  size_t dir_len = strlen(directory);
  size_t pattern_len = strlen(pattern);

#if defined(_WIN32) || defined(_WIN64)
  bool separator =
      dir_len > 0 &&
      directory[dir_len - 1] != '/' &&
      directory[dir_len - 1] != '\\';
#else
  bool separator =
      dir_len > 0 &&
      directory[dir_len - 1] != '/';
#endif

  size_t total =
      dir_len +
      (separator ? 1 : 0) +
      pattern_len;

  char *result = malloc(total + 1);

  if (!result)
    return NULL;

  memcpy(result, directory, dir_len);

  size_t offset = dir_len;

  if (separator) {
#if defined(_WIN32) || defined(_WIN64)
    result[offset++] = '\\';
#else
    result[offset++] = '/';
#endif
  }

  memcpy(result + offset, pattern, pattern_len);

  result[total] = '\0';

  return result;
}

ARGON_FUNCTION(temp_file, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct string pattern_str =
      api->argon_to_string(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *pattern = malloc(pattern_str.length + 1);

  if (!pattern)
    return api->throw_argon_error(
        err,
        api->RuntimeError,
        "out of memory");

  memcpy(pattern, pattern_str.data, pattern_str.length);
  pattern[pattern_str.length] = '\0';

  const char *temp_directory =
      argon_get_temp_directory();

  if (!temp_directory) {
    free(pattern);

    return api->throw_argon_error(
        err,
        argv[1],
        "failed to determine temporary directory");
  }

  /*
   * Try multiple names in case of a collision.
   */
  for (int attempt = 0; attempt < 100; attempt++) {

    char *candidate =
    argon_temp_make_candidate(pattern);

    if (!candidate) {
      free(pattern);

      return api->throw_argon_error(
          err,
          api->RuntimeError,
          "out of memory");
    }

    char *path = argon_temp_join_path(
        temp_directory,
        candidate);

    free(candidate);

    if (!path) {
      free(pattern);

      return api->throw_argon_error(
          err,
          api->RuntimeError,
          "out of memory");
    }

#if defined(_WIN32) || defined(_WIN64)

    HANDLE handle = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY,
        NULL);

    if (handle != INVALID_HANDLE_VALUE) {
      CloseHandle(handle);

      ArgonObject *result =
          api->string_to_argon(
              (struct string){path, strlen(path)});

      free(path);
      free(pattern);

      return result;
    }

    DWORD error = GetLastError();

    free(path);

    if (error != ERROR_FILE_EXISTS &&
        error != ERROR_ALREADY_EXISTS) {

      free(pattern);

      return api->throw_argon_error(
          err,
          argv[1],
          "failed to create temporary file: %lu",
          error);
    }

#else

    /*
     * O_CREAT | O_EXCL makes creation atomic and prevents us from
     * accidentally opening an existing file.
     */
    int fd = open(
        path,
        O_RDWR | O_CREAT | O_EXCL,
        0600);

    if (fd >= 0) {
      close(fd);

      ArgonObject *result =
          api->string_to_argon(
              (struct string){path, strlen(path)});

      free(path);
      free(pattern);

      return result;
    }

    int error = errno;

    free(path);

    if (error != EEXIST) {
      free(pattern);

      return api->throw_argon_error(
          err,
          argv[1],
          "failed to create temporary file: %s",
          strerror(error));
    }

#endif
  }

  free(pattern);

  return api->throw_argon_error(
      err,
      argv[1],
      "failed to create a unique temporary file");
})

ARGON_FUNCTION(temp_dir, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct string pattern_str =
      api->argon_to_string(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *pattern = malloc(pattern_str.length + 1);

  if (!pattern)
    return api->throw_argon_error(
        err,
        api->RuntimeError,
        "out of memory");

  memcpy(pattern, pattern_str.data, pattern_str.length);
  pattern[pattern_str.length] = '\0';

  const char *temp_directory =
      argon_get_temp_directory();

  if (!temp_directory) {
    free(pattern);

    return api->throw_argon_error(
        err,
        argv[1],
        "failed to determine temporary directory");
  }

  /*
   * Try multiple names in case of a collision.
   */
  for (int attempt = 0; attempt < 100; attempt++) {

    char *candidate =
    argon_temp_make_candidate(pattern);

    if (!candidate) {
      free(pattern);

      return api->throw_argon_error(
          err,
          api->RuntimeError,
          "out of memory");
    }

    char *path = argon_temp_join_path(
        temp_directory,
        candidate);

    free(candidate);

    if (!path) {
      free(pattern);

      return api->throw_argon_error(
          err,
          api->RuntimeError,
          "out of memory");
    }

#if defined(_WIN32) || defined(_WIN64)

    if (CreateDirectoryA(path, NULL)) {

      ArgonObject *result =
          api->string_to_argon(
              (struct string){path, strlen(path)});

      free(path);
      free(pattern);

      return result;
    }

    DWORD error = GetLastError();

    free(path);

    if (error != ERROR_ALREADY_EXISTS) {
      free(pattern);

      return api->throw_argon_error(
          err,
          argv[1],
          "failed to create temporary directory: %lu",
          error);
    }

#else

    if (mkdir(path, 0700) == 0) {

      ArgonObject *result =
          api->string_to_argon(
              (struct string){path, strlen(path)});

      free(path);
      free(pattern);

      return result;
    }

    int error = errno;

    free(path);

    if (error != EEXIST) {
      free(pattern);

      return api->throw_argon_error(
          err,
          argv[1],
          "failed to create temporary directory: %s",
          strerror(error));
    }

#endif
  }

  free(pattern);

  return api->throw_argon_error(
      err,
      argv[1],
      "failed to create a unique temporary directory");
})

INIT_ARGON_MODULE({
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
  REGISTER_ARGON_FUNCTION(mkdir)
  REGISTER_ARGON_FUNCTION(mkdir_p)
  REGISTER_ARGON_FUNCTION(temp_file)
  REGISTER_ARGON_FUNCTION(temp_dir)
})