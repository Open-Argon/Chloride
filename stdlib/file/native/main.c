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
#include <bcrypt.h>

#else

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/random.h>
#include <unistd.h>
#endif
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

#define ARGON_TEMP_RANDOM_CHARS                                                \
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

static bool argon_temp_random_bytes(
    unsigned char *data,
    size_t length) {

  if (length == 0)
    return true;

#if defined(_WIN32) || defined(_WIN64)

  if (length > ULONG_MAX)
    return false;

  NTSTATUS status = BCryptGenRandom(
      NULL,
      data,
      (ULONG)length,
      BCRYPT_USE_SYSTEM_PREFERRED_RNG);

  return status == STATUS_SUCCESS;

#elif defined(__linux__)

  size_t offset = 0;

  while (offset < length) {

    ssize_t result = getrandom(
        data + offset,
        length - offset,
        0);

    if (result < 0) {
      if (errno == EINTR)
        continue;

      return false;
    }

    if (result == 0)
      return false;

    offset += (size_t)result;
  }

  return true;

#else

  arc4random_buf(data, length);
  return true;

#endif
}

static bool argon_temp_fill_random(
    char *data,
    size_t length) {

  if (length == 0)
    return true;

  const size_t chars =
      sizeof(ARGON_TEMP_RANDOM_CHARS) - 1;

  /*
   * 248 is the largest multiple of 62 below 256.
   * Rejecting 248..255 avoids modulo bias.
   */
  unsigned char random[256];

  size_t written = 0;

  while (written < length) {

    size_t requested = length - written;

    if (requested > sizeof(random))
      requested = sizeof(random);

    if (!argon_temp_random_bytes(
            random,
            requested))
      return false;

    for (size_t i = 0; i < requested; i++) {

      unsigned char value = random[i];

      if (value >= 248)
        continue;

      data[written++] =
          ARGON_TEMP_RANDOM_CHARS[
              value % chars];

      if (written == length)
        break;
    }
  }

  return true;
}

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

  size_t length = prefix_len + ARGON_TEMP_RANDOM_LENGTH + suffix_len;

  char *result = malloc(length + 1);

  if (!result)
    return NULL;

  memcpy(result, pattern, prefix_len);

  argon_temp_fill_random(result + prefix_len, ARGON_TEMP_RANDOM_LENGTH);

  memcpy(result + prefix_len + ARGON_TEMP_RANDOM_LENGTH, star + 1, suffix_len);

  result[length] = '\0';

  return result;
}

static char *argon_temp_join_path(const char *directory, const char *pattern) {

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
  bool separator = dir_len > 0 && directory[dir_len - 1] != '/' &&
                   directory[dir_len - 1] != '\\';
#else
  bool separator = dir_len > 0 && directory[dir_len - 1] != '/';
#endif

  size_t total = dir_len + (separator ? 1 : 0) + pattern_len;

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

  struct string pattern_str = api->argon_to_string(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *pattern = malloc(pattern_str.length + 1);

  if (!pattern)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");

  memcpy(pattern, pattern_str.data, pattern_str.length);
  pattern[pattern_str.length] = '\0';

  const char *temp_directory = argon_get_temp_directory();

  if (!temp_directory) {
    free(pattern);

    return api->throw_argon_error(err, argv[1],
                                  "failed to determine temporary directory");
  }

  /*
   * Try multiple names in case of a collision.
   */
  for (int attempt = 0; attempt < 100; attempt++) {

    char *candidate = argon_temp_make_candidate(pattern);

    if (!candidate) {
      free(pattern);

      return api->throw_argon_error(err, api->RuntimeError, "out of memory");
    }

    char *path = argon_temp_join_path(temp_directory, candidate);

    free(candidate);

    if (!path) {
      free(pattern);

      return api->throw_argon_error(err, api->RuntimeError, "out of memory");
    }

#if defined(_WIN32) || defined(_WIN64)

    HANDLE handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL);

    if (handle != INVALID_HANDLE_VALUE) {
      CloseHandle(handle);

      ArgonObject *result =
          api->string_to_argon((struct string){path, strlen(path)});

      free(path);
      free(pattern);

      return result;
    }

    DWORD error = GetLastError();

    free(path);

    if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) {

      free(pattern);

      return api->throw_argon_error(
          err, argv[1], "failed to create temporary file: %lu", error);
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

  return api->throw_argon_error(err, argv[1],
                                "failed to create a unique temporary file");
})

ARGON_FUNCTION(temp_dir, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct string pattern_str = api->argon_to_string(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *pattern = malloc(pattern_str.length + 1);

  if (!pattern)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");

  memcpy(pattern, pattern_str.data, pattern_str.length);
  pattern[pattern_str.length] = '\0';

  const char *temp_directory = argon_get_temp_directory();

  if (!temp_directory) {
    free(pattern);

    return api->throw_argon_error(err, argv[1],
                                  "failed to determine temporary directory");
  }

  /*
   * Try multiple names in case of a collision.
   */
  for (int attempt = 0; attempt < 100; attempt++) {

    char *candidate = argon_temp_make_candidate(pattern);

    if (!candidate) {
      free(pattern);

      return api->throw_argon_error(err, api->RuntimeError, "out of memory");
    }

    char *path = argon_temp_join_path(temp_directory, candidate);

    free(candidate);

    if (!path) {
      free(pattern);

      return api->throw_argon_error(err, api->RuntimeError, "out of memory");
    }

#if defined(_WIN32) || defined(_WIN64)

    if (CreateDirectoryA(path, NULL)) {

      ArgonObject *result =
          api->string_to_argon((struct string){path, strlen(path)});

      free(path);
      free(pattern);

      return result;
    }

    DWORD error = GetLastError();

    free(path);

    if (error != ERROR_ALREADY_EXISTS) {
      free(pattern);

      return api->throw_argon_error(
          err, argv[1], "failed to create temporary directory: %lu", error);
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
      err, argv[1], "failed to create a unique temporary directory");
})

static char *argon_path_to_c_string(ArgonNativeAPI *api, ArgonObject *obj,
                                    ArgonError *err) {

  struct string str = api->argon_to_string(obj, err);

  if (api->is_error(err))
    return NULL;

  char *path = malloc(str.length + 1);

  if (!path)
    return NULL;

  memcpy(path, str.data, str.length);
  path[str.length] = '\0';

  return path;
}

static bool argon_is_directory(const char *path) {

#if defined(_WIN32) || defined(_WIN64)

  struct _stat st;

  if (_stat(path, &st) != 0)
    return false;

  return (st.st_mode & _S_IFMT) == _S_IFDIR;

#else

  struct stat st;

  if (stat(path, &st) != 0)
    return false;

  return S_ISDIR(st.st_mode);

#endif
}

static bool argon_is_file(const char *path) {

#if defined(_WIN32) || defined(_WIN64)

  struct _stat st;

  if (_stat(path, &st) != 0)
    return false;

  return (st.st_mode & _S_IFMT) == _S_IFREG;

#else

  struct stat st;

  if (stat(path, &st) != 0)
    return false;

  return S_ISREG(st.st_mode);

#endif
}

static char *argon_path_join(const char *parent, const char *child) {

  size_t parent_len = strlen(parent);
  size_t child_len = strlen(child);

#if defined(_WIN32) || defined(_WIN64)

  bool separator = parent_len > 0 && parent[parent_len - 1] != '/' &&
                   parent[parent_len - 1] != '\\';

  size_t length = parent_len + (separator ? 1 : 0) + child_len;

  char *result = malloc(length + 1);

  if (!result)
    return NULL;

  memcpy(result, parent, parent_len);

  size_t offset = parent_len;

  if (separator)
    result[offset++] = '\\';

  memcpy(result + offset, child, child_len);

#else

  bool separator = parent_len > 0 && parent[parent_len - 1] != '/';

  size_t length = parent_len + (separator ? 1 : 0) + child_len;

  char *result = malloc(length + 1);

  if (!result)
    return NULL;

  memcpy(result, parent, parent_len);

  size_t offset = parent_len;

  if (separator)
    result[offset++] = '/';

  memcpy(result + offset, child, child_len);

#endif

  result[length] = '\0';

  return result;
}

static bool argon_copy_file_native(const char *src, const char *dst,
                                   int *error_number) {

#if defined(_WIN32) || defined(_WIN64)

  if (CopyFileA(src, dst, TRUE))
    return true;

  if (error_number)
    *error_number = (int)GetLastError();

  return false;

#else

  int in_fd = open(src, O_RDONLY);

  if (in_fd < 0) {
    if (error_number)
      *error_number = errno;

    return false;
  }

  struct stat st;

  if (fstat(in_fd, &st) != 0) {
    if (error_number)
      *error_number = errno;

    close(in_fd);
    return false;
  }

  int out_fd = open(dst, O_WRONLY | O_CREAT | O_EXCL, st.st_mode & 0777);

  if (out_fd < 0) {
    if (error_number)
      *error_number = errno;

    close(in_fd);
    return false;
  }

  char buffer[64 * 1024];

  while (true) {

    ssize_t bytes_read = read(in_fd, buffer, sizeof(buffer));

    if (bytes_read == 0)
      break;

    if (bytes_read < 0) {

      if (error_number)
        *error_number = errno;

      close(in_fd);
      close(out_fd);
      unlink(dst);

      return false;
    }

    ssize_t offset = 0;

    while (offset < bytes_read) {

      ssize_t bytes_written =
          write(out_fd, buffer + offset, (size_t)(bytes_read - offset));

      if (bytes_written <= 0) {

        if (error_number)
          *error_number = errno;

        close(in_fd);
        close(out_fd);
        unlink(dst);

        return false;
      }

      offset += bytes_written;
    }
  }

  close(in_fd);

  if (close(out_fd) != 0) {

    if (error_number)
      *error_number = errno;

    unlink(dst);

    return false;
  }

  return true;

#endif
}

static bool argon_copy_directory_native(const char *src, const char *dst,
                                        int *error_number) {

  if (!argon_is_directory(src)) {

    if (error_number)
      *error_number = ENOTDIR;

    return false;
  }

#if defined(_WIN32) || defined(_WIN64)

  if (!CreateDirectoryA(dst, NULL)) {

    DWORD error = GetLastError();

    if (error != ERROR_ALREADY_EXISTS) {

      if (error_number)
        *error_number = (int)error;

      return false;
    }

    if (!argon_is_directory(dst)) {

      if (error_number)
        *error_number = (int)error;

      return false;
    }
  }

  char *pattern = argon_path_join(src, "*");

  if (!pattern) {
    if (error_number)
      *error_number = ENOMEM;

    return false;
  }

  WIN32_FIND_DATAA data;
  HANDLE find = FindFirstFileA(pattern, &data);

  free(pattern);

  if (find == INVALID_HANDLE_VALUE) {

    DWORD error = GetLastError();

    if (error == ERROR_FILE_NOT_FOUND)
      return true;

    if (error_number)
      *error_number = (int)error;

    return false;
  }

  bool success = true;

  do {

    if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
      continue;

    char *src_path = argon_path_join(src, data.cFileName);

    char *dst_path = argon_path_join(dst, data.cFileName);

    if (!src_path || !dst_path) {

      free(src_path);
      free(dst_path);

      if (error_number)
        *error_number = ENOMEM;

      success = false;
      break;
    }

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

      if (!argon_copy_directory_native(src_path, dst_path, error_number)) {

        success = false;
      }

    } else {

      if (!argon_copy_file_native(src_path, dst_path, error_number)) {

        success = false;
      }
    }

    free(src_path);
    free(dst_path);

    if (!success)
      break;

  } while (FindNextFileA(find, &data));

  FindClose(find);

  return success;

#else

  if (mkdir(dst, 0755) != 0) {

    if (errno != EEXIST || !argon_is_directory(dst)) {

      if (error_number)
        *error_number = errno;

      return false;
    }
  }

  DIR *directory = opendir(src);

  if (!directory) {

    if (error_number)
      *error_number = errno;

    return false;
  }

  bool success = true;

  struct dirent *entry;

  while ((entry = readdir(directory)) != NULL) {

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char *src_path = argon_path_join(src, entry->d_name);

    char *dst_path = argon_path_join(dst, entry->d_name);

    if (!src_path || !dst_path) {

      free(src_path);
      free(dst_path);

      if (error_number)
        *error_number = ENOMEM;

      success = false;
      break;
    }

    if (argon_is_directory(src_path)) {

      if (!argon_copy_directory_native(src_path, dst_path, error_number)) {

        success = false;
      }

    } else if (argon_is_file(src_path)) {

      if (!argon_copy_file_native(src_path, dst_path, error_number)) {

        success = false;
      }
    }

    free(src_path);
    free(dst_path);

    if (!success)
      break;
  }

  closedir(directory);

  return success;

#endif
}

static bool argon_remove_directory_native(const char *path, int *error_number) {

#if defined(_WIN32) || defined(_WIN64)

  char *pattern = argon_path_join(path, "*");

  if (!pattern) {
    if (error_number)
      *error_number = ENOMEM;

    return false;
  }

  WIN32_FIND_DATAA data;

  HANDLE find = FindFirstFileA(pattern, &data);

  free(pattern);

  if (find != INVALID_HANDLE_VALUE) {

    do {

      if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
        continue;

      char *child = argon_path_join(path, data.cFileName);

      if (!child) {

        if (error_number)
          *error_number = ENOMEM;

        FindClose(find);
        return false;
      }

      bool result;

      if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        result = argon_remove_directory_native(child, error_number);

      } else {

        result = DeleteFileA(child);

        if (!result && error_number)
          *error_number = (int)GetLastError();
      }

      free(child);

      if (!result) {
        FindClose(find);
        return false;
      }

    } while (FindNextFileA(find, &data));

    FindClose(find);
  }

  if (!RemoveDirectoryA(path)) {

    if (error_number)
      *error_number = (int)GetLastError();

    return false;
  }

  return true;

#else

  DIR *directory = opendir(path);

  if (!directory) {

    if (error_number)
      *error_number = errno;

    return false;
  }

  struct dirent *entry;

  while ((entry = readdir(directory)) != NULL) {

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char *child = argon_path_join(path, entry->d_name);

    if (!child) {

      if (error_number)
        *error_number = ENOMEM;

      closedir(directory);
      return false;
    }

    bool result;

    if (argon_is_directory(child)) {

      result = argon_remove_directory_native(child, error_number);

    } else {

      result = unlink(child) == 0;

      if (!result && error_number)
        *error_number = errno;
    }

    free(child);

    if (!result) {
      closedir(directory);
      return false;
    }
  }

  closedir(directory);

  if (rmdir(path) != 0) {

    if (error_number)
      *error_number = errno;

    return false;
  }

  return true;

#endif
}

ARGON_FUNCTION(copy_file, {
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  char *src = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *dst = argon_path_to_c_string(api, argv[1], err);

  if (api->is_error(err)) {
    free(src);
    return api->ARGON_NULL;
  }

  if (!argon_is_file(src)) {

    ArgonObject *result =
        api->throw_argon_error(err, argv[2], "source is not a file: %s", src);

    free(src);
    free(dst);

    return result;
  }

  int error_number = 0;

  if (!argon_copy_file_native(src, dst, &error_number)) {

    ArgonObject *result = api->throw_argon_error(
        err, argv[2], "failed to copy file: %s", strerror(error_number));

    free(src);
    free(dst);

    return result;
  }

  free(src);
  free(dst);

  return api->ARGON_NULL;
})

ARGON_FUNCTION(copy_dir, {
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  char *src = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *dst = argon_path_to_c_string(api, argv[1], err);

  if (api->is_error(err)) {
    free(src);
    return api->ARGON_NULL;
  }

  if (!argon_is_directory(src)) {

    ArgonObject *result = api->throw_argon_error(
        err, argv[2], "source is not a directory: %s", src);

    free(src);
    free(dst);

    return result;
  }

  int error_number = 0;

  if (!argon_copy_directory_native(src, dst, &error_number)) {

    ArgonObject *result = api->throw_argon_error(
        err, argv[2], "failed to copy directory: %s", strerror(error_number));

    free(src);
    free(dst);

    return result;
  }

  free(src);
  free(dst);

  return api->ARGON_NULL;
})

ARGON_FUNCTION(move, {
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  char *src = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *dst = argon_path_to_c_string(api, argv[1], err);

  if (api->is_error(err)) {
    free(src);
    return api->ARGON_NULL;
  }

#if defined(_WIN32) || defined(_WIN64)

  if (MoveFileA(src, dst)) {

    free(src);
    free(dst);

    return api->ARGON_NULL;
  }

  DWORD move_error = GetLastError();

  /*
   * ERROR_NOT_SAME_DEVICE means the source and destination are
   * on different drives/filesystems.
   */
  if (move_error != ERROR_NOT_SAME_DEVICE) {

    ArgonObject *result = api->throw_argon_error(
        err, argv[2], "failed to move '%s' to '%s': %lu", src, dst, move_error);

    free(src);
    free(dst);

    return result;
  }

#else

  if (rename(src, dst) == 0) {

    free(src);
    free(dst);

    return api->ARGON_NULL;
  }

  int rename_error = errno;

  /*
   * EXDEV means the paths are on different filesystems.
   */
  if (rename_error != EXDEV) {

    ArgonObject *result =
        api->throw_argon_error(
            err,
            argv[2],
            "failed to move '%s' to '%s': %s",
            src,
            dst,
            strerror(rename_error));

    free(src);
    free(dst);

    return result;
  }

#endif

  /*
   * Cross-filesystem move:
   *
   * file -> copy file + remove file
   * directory -> copy directory + remove directory
   */
  int error_number = 0;
  bool success = false;

  if (argon_is_directory(src)) {

    success = argon_copy_directory_native(src, dst, &error_number);

    if (success)
      success = argon_remove_directory_native(src, &error_number);

  } else if (argon_is_file(src)) {

    success = argon_copy_file_native(src, dst, &error_number);

    if (success) {

#if defined(_WIN32) || defined(_WIN64)
      success = DeleteFileA(src);

      if (!success)
        error_number = (int)GetLastError();
#else
      success = unlink(src) == 0;

      if (!success)
        error_number = errno;
#endif
    }

  } else {

    ArgonObject *result =
        api->throw_argon_error(err, argv[2], "source does not exist: %s", src);

    free(src);
    free(dst);

    return result;
  }

  if (!success) {

    ArgonObject *result =
        api->throw_argon_error(err, argv[2], "failed to move '%s' to '%s': %s",
                               src, dst, strerror(error_number));

    free(src);
    free(dst);

    return result;
  }

  free(src);
  free(dst);

  return api->ARGON_NULL;
})

static bool argon_tree_add_path(ArgonNativeAPI *api, ArgonObject ***items,
                                size_t *size, size_t *capacity,
                                const char *path, ArgonError *err) {

  if (*size >= *capacity) {

    size_t new_capacity = *capacity == 0 ? 16 : *capacity * 2;

    ArgonObject **new_items =
        realloc(*items, new_capacity * sizeof(ArgonObject *));

    if (!new_items) {

      api->throw_argon_error(err, api->RuntimeError, "out of memory");

      return false;
    }

    *items = new_items;
    *capacity = new_capacity;
  }

  ArgonObject *string =
      api->string_to_argon((struct string){(char *)path, strlen(path)});

  (*items)[(*size)++] = string;

  return true;
}

static bool argon_tree_directory_native(ArgonNativeAPI *api, const char *root,
                                        ArgonObject ***items, size_t *size,
                                        size_t *capacity, ArgonError *err) {

#if defined(_WIN32) || defined(_WIN64)

  char *pattern = argon_path_join(root, "*");

  if (!pattern) {

    api->throw_argon_error(err, api->RuntimeError, "out of memory");

    return false;
  }

  WIN32_FIND_DATAA data;

  HANDLE find = FindFirstFileA(pattern, &data);

  free(pattern);

  if (find == INVALID_HANDLE_VALUE) {

    DWORD error = GetLastError();

    if (error == ERROR_FILE_NOT_FOUND)
      return true;

    api->throw_argon_error(err, api->PathError,
                           "failed to read directory '%s': %lu", root, error);

    return false;
  }

  bool success = true;

  do {

    if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
      continue;

    char *path = argon_path_join(root, data.cFileName);

    if (!path) {

      api->throw_argon_error(err, api->RuntimeError, "out of memory");

      success = false;
      break;
    }

    if (!argon_tree_add_path(api, items, size, capacity, path, err)) {

      free(path);
      success = false;
      break;
    }

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

      if (!argon_tree_directory_native(api, path, items, size, capacity, err)) {

        free(path);
        success = false;
        break;
      }
    }

    free(path);

  } while (FindNextFileA(find, &data));

  FindClose(find);

  return success;

#else

  DIR *directory = opendir(root);

  if (!directory) {

    api->throw_argon_error(err, api->PathError,
                           "failed to read directory '%s': %s", root,
                           strerror(errno));

    return false;
  }

  struct dirent *entry;

  while ((entry = readdir(directory)) != NULL) {

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char *path = argon_path_join(root, entry->d_name);

    if (!path) {

      closedir(directory);

      api->throw_argon_error(err, api->RuntimeError, "out of memory");

      return false;
    }

    if (!argon_tree_add_path(api, items, size, capacity, path, err)) {

      free(path);
      closedir(directory);

      return false;
    }

    if (argon_is_directory(path)) {

      if (!argon_tree_directory_native(api, path, items, size, capacity, err)) {

        free(path);
        closedir(directory);

        return false;
      }
    }

    free(path);
  }

  closedir(directory);

  return true;

#endif
}

ARGON_FUNCTION(tree_dir, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  char *root = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  if (!argon_is_directory(root)) {

    ArgonObject *result =
        api->throw_argon_error(err, argv[1], "not a directory: %s", root);

    free(root);

    return result;
  }

  ArgonObject **items = NULL;
  size_t size = 0;
  size_t capacity = 0;

  bool success =
      argon_tree_directory_native(api, root, &items, &size, &capacity, err);

  free(root);

  if (!success) {
    free(items);
    return api->ARGON_NULL;
  }

  ArgonObject *result = api->create_argon_array(items, size);

  free(items);

  return result;
})

ARGON_FUNCTION(delete_file, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  char *path = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  if (!argon_is_file(path)) {
    ArgonObject *result =
        api->throw_argon_error(err, argv[1], "not a file: %s", path);

    free(path);
    return result;
  }

#if defined(_WIN32) || defined(_WIN64)

  if (!DeleteFileA(path)) {
    DWORD error = GetLastError();

    ArgonObject *result = api->throw_argon_error(
        err, argv[1], "failed to delete file '%s': %lu", path, error);

    free(path);
    return result;
  }

#else

  if (unlink(path) != 0) {
    int error = errno;

    ArgonObject *result =
        api->throw_argon_error(
            err,
            argv[1],
            "failed to delete file '%s': %s",
            path,
            strerror(error));

    free(path);
    return result;
  }

#endif

  free(path);

  return api->ARGON_NULL;
})

ARGON_FUNCTION(delete_dir, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  char *path = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  if (!argon_is_directory(path)) {
    ArgonObject *result =
        api->throw_argon_error(err, argv[1], "not a directory: %s", path);

    free(path);
    return result;
  }

  int error_number = 0;

  if (!argon_remove_directory_native(path, &error_number)) {

    ArgonObject *result = api->throw_argon_error(
        err, argv[1], "failed to delete directory '%s': %s", path,
        strerror(error_number));

    free(path);
    return result;
  }

  free(path);

  return api->ARGON_NULL;
})

static bool argon_move_contents_native(const char *src, const char *dst,
                                       int *error_number) {

  if (!argon_is_directory(src)) {
    if (error_number)
      *error_number = ENOTDIR;

    return false;
  }

  if (!argon_is_directory(dst)) {
    if (error_number)
      *error_number = ENOTDIR;

    return false;
  }

#if defined(_WIN32) || defined(_WIN64)

  char *pattern = argon_path_join(src, "*");

  if (!pattern) {
    if (error_number)
      *error_number = ENOMEM;

    return false;
  }

  WIN32_FIND_DATAA data;
  HANDLE find = FindFirstFileA(pattern, &data);

  free(pattern);

  if (find == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();

    if (error == ERROR_FILE_NOT_FOUND)
      return true;

    if (error_number)
      *error_number = (int)error;

    return false;
  }

  bool success = true;

  do {
    if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
      continue;

    char *src_path = argon_path_join(src, data.cFileName);

    char *dst_path = argon_path_join(dst, data.cFileName);

    if (!src_path || !dst_path) {
      free(src_path);
      free(dst_path);

      if (error_number)
        *error_number = ENOMEM;

      success = false;
      break;
    }

    bool result;

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

      /*
       * Try a native rename first.
       */
      result = MoveFileA(src_path, dst_path);

      if (!result && GetLastError() == ERROR_NOT_SAME_DEVICE) {

        result = argon_copy_directory_native(src_path, dst_path, error_number);

        if (result)
          result = argon_remove_directory_native(src_path, error_number);
      } else if (!result) {
        if (error_number)
          *error_number = (int)GetLastError();
      }

    } else {

      result = MoveFileA(src_path, dst_path);

      if (!result && GetLastError() == ERROR_NOT_SAME_DEVICE) {

        result = argon_copy_file_native(src_path, dst_path, error_number);

        if (result) {
          result = DeleteFileA(src_path);

          if (!result && error_number)
            *error_number = (int)GetLastError();
        }

      } else if (!result) {
        if (error_number)
          *error_number = (int)GetLastError();
      }
    }

    free(src_path);
    free(dst_path);

    if (!result) {
      success = false;
      break;
    }

  } while (FindNextFileA(find, &data));

  FindClose(find);

  return success;

#else

  DIR *directory = opendir(src);

  if (!directory) {
    if (error_number)
      *error_number = errno;

    return false;
  }

  struct dirent *entry;
  bool success = true;

  while ((entry = readdir(directory)) != NULL) {

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char *src_path = argon_path_join(src, entry->d_name);

    char *dst_path = argon_path_join(dst, entry->d_name);

    if (!src_path || !dst_path) {
      free(src_path);
      free(dst_path);

      if (error_number)
        *error_number = ENOMEM;

      success = false;
      break;
    }

    /*
     * rename() works when source and destination are on
     * the same filesystem. This is the fast path.
     */
    if (rename(src_path, dst_path) != 0) {

      int rename_error = errno;

      if (rename_error == EXDEV) {

        if (argon_is_directory(src_path)) {

          if (argon_copy_directory_native(src_path, dst_path, error_number)) {

            if (!argon_remove_directory_native(src_path, error_number)) {

              success = false;
            }

          } else {
            success = false;
          }

        } else {

          if (argon_copy_file_native(src_path, dst_path, error_number)) {

            if (unlink(src_path) != 0) {
              if (error_number)
                *error_number = errno;

              success = false;
            }

          } else {
            success = false;
          }
        }

      } else {

        if (error_number)
          *error_number = rename_error;

        success = false;
      }
    }

    free(src_path);
    free(dst_path);

    if (!success)
      break;
  }

  closedir(directory);

  return success;

#endif
}

ARGON_FUNCTION(move_contents, {
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  char *src = argon_path_to_c_string(api, argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  char *dst = argon_path_to_c_string(api, argv[1], err);

  if (api->is_error(err)) {
    free(src);
    return api->ARGON_NULL;
  }

  int error_number = 0;

  if (!argon_move_contents_native(src, dst, &error_number)) {

    ArgonObject *result = api->throw_argon_error(
        err, argv[2], "failed to move contents of '%s' to '%s': %s", src, dst,
        strerror(error_number));

    free(src);
    free(dst);

    return result;
  }

  free(src);
  free(dst);

  return api->ARGON_NULL;
})

INIT_ARGON_MODULE({
  REGISTER_ARGON_FUNCTION(open_handle);
  REGISTER_ARGON_FUNCTION(read_all);
  REGISTER_ARGON_FUNCTION(read);
  REGISTER_ARGON_FUNCTION(write);
  REGISTER_ARGON_FUNCTION(close);
  REGISTER_ARGON_FUNCTION(seek);
  REGISTER_ARGON_FUNCTION(tell);
  REGISTER_ARGON_FUNCTION(flush);
  REGISTER_ARGON_FUNCTION(file_size);
  REGISTER_ARGON_FUNCTION(path_type);
  REGISTER_ARGON_FUNCTION(open_stdout);
  REGISTER_ARGON_FUNCTION(open_stdin);
  REGISTER_ARGON_FUNCTION(open_stderr);
  REGISTER_ARGON_FUNCTION(open_stdnull);
  REGISTER_ARGON_FUNCTION(mkdir);
  REGISTER_ARGON_FUNCTION(mkdir_p);
  REGISTER_ARGON_FUNCTION(temp_file);
  REGISTER_ARGON_FUNCTION(temp_dir);
  REGISTER_ARGON_FUNCTION(move);
  REGISTER_ARGON_FUNCTION(copy_file);
  REGISTER_ARGON_FUNCTION(copy_dir);
  REGISTER_ARGON_FUNCTION(tree_dir);
  REGISTER_ARGON_FUNCTION(delete_file);
  REGISTER_ARGON_FUNCTION(delete_dir);
  REGISTER_ARGON_FUNCTION(move_contents);
})