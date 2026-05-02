// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "ArgonFunction.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  FILE *fp;
  int is_open;
} FileHandle;

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

  if (fclose(handle->fp) != 0)
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

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;
  api->register_ArgonObject(
      reg, "open_handle",
      api->create_argon_native_function("open_handle", open_handle));
  api->register_ArgonObject(
      reg, "read_all", api->create_argon_native_function("read_all", read_all));
  api->register_ArgonObject(reg, "read",
                            api->create_argon_native_function("read", read));
  api->register_ArgonObject(reg, "write",
                            api->create_argon_native_function("write", write));
  api->register_ArgonObject(reg, "close",
                            api->create_argon_native_function("close", close));
  api->register_ArgonObject(reg, "seek",
                            api->create_argon_native_function("seek", seek));
  api->register_ArgonObject(reg, "tell",
                            api->create_argon_native_function("tell", tell));
  api->register_ArgonObject(reg, "flush",
                            api->create_argon_native_function("flush", flush));
}