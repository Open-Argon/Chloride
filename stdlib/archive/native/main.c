// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "../../../external/libarchive/libarchive/archive.h"
#include "../../../external/libarchive/libarchive/archive_entry.h"
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------------
// Handle wrappers
//
// We can't store a raw struct archive* in an ArgonObject buffer
// alone, because reading also needs an open struct archive_entry*
// cursor and writing needs to know whether it's already closed.
// Mirrors regex's "buffer holds a pointer" trick but wraps it in a
// small heap struct instead of storing the pointer directly, so we
// have somewhere to hang the entry cursor / open-state flag.
// ------------------------------------------------------------

typedef struct {
  struct archive *a;
  struct archive_entry *entry; // current entry cursor for readers, unused for writers
  int closed;
} ar_handle;

// Helper: pull an ar_handle* out of an ArgonObject buffer
static ar_handle *get_handle(ArgonObject *obj, ArErr *err, ArgonNativeAPI *api) {
  struct buffer buf = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err))
    return NULL;
  return *(ar_handle **)buf.data;
}

// Helper: wrap an ar_handle* in a fresh ArgonObject buffer
static ArgonObject *wrap_handle(ArgonNativeAPI *api, ArErr *err, ar_handle *h) {
  ArgonObject *obj = api->create_argon_buffer(sizeof(ar_handle *));
  struct buffer buf = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err))
    return NULL;

  *(ar_handle **)buf.data = h;
  return obj;
}

// Helper: raise ArchiveError with libarchive's own error string
static ArgonObject *throw_archive_error(ArgonNativeAPI *api, ArErr *err,
                                        ArgonObject *err_type,
                                        struct archive *a,
                                        const char *fallback) {
  const char *msg = a ? archive_error_string(a) : NULL;
  return api->throw_argon_error(err, err_type, "%s", msg ? msg : fallback);
}

// ------------------------------------------------------------
// Reader
// ------------------------------------------------------------

// reader_open(path, ArchiveError) -> handle
// Opens path for reading with every format/filter libarchive supports
// auto-detected (tar, zip, 7z, iso9660, cpio... + gz/bz2/xz/zstd/etc).
ARGON_FUNCTION(reader_open, {
  struct string path = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[1];

  struct archive *a = archive_read_new();
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);

  // path.data isn't guaranteed NUL-terminated by Argon's string type;
  // build a temporary NUL-terminated copy for the C API.
  char *cpath = malloc(path.length + 1);
  if (!cpath) {
    archive_read_free(a);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  memcpy(cpath, path.data, path.length);
  cpath[path.length] = '\0';

  int rc = archive_read_open_filename(a, cpath, 10240);
  free(cpath);

  if (rc != ARCHIVE_OK) {
    ArgonObject *res = throw_archive_error(api, err, err_type, a,
                                           "Failed to open archive for reading");
    archive_read_free(a);
    return res;
  }

  ar_handle *h = malloc(sizeof(ar_handle));
  if (!h) {
    archive_read_free(a);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  h->a = a;
  h->entry = NULL;
  h->closed = 0;

  return wrap_handle(api, err, h);
})

// reader_open_memory(bytes, ArchiveError) -> handle
// Same as reader_open but reads from an in-memory byte string instead
// of a file on disk.
ARGON_FUNCTION(reader_open_memory, {
  struct buffer data = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[1];

  struct archive *a = archive_read_new();
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);

  // NOTE: archive_read_open_memory does not take ownership/copy of the
  // buffer; the ArgonObject argv[0] must be kept alive by the caller
  // (init.ar) for the lifetime of the reader.
  int rc = archive_read_open_memory(a, data.data, data.size);
  if (rc != ARCHIVE_OK) {
    ArgonObject *res = throw_archive_error(api, err, err_type, a,
                                           "Failed to open archive from memory");
    archive_read_free(a);
    return res;
  }

  ar_handle *h = malloc(sizeof(ar_handle));
  if (!h) {
    archive_read_free(a);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  h->a = a;
  h->entry = NULL;
  h->closed = 0;

  return wrap_handle(api, err, h);
})

// reader_next(handle) -> bool
// Advances to the next entry. Returns false at end of archive.
ARGON_FUNCTION(reader_next, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h)
    return api->ARGON_NULL;

  int rc = archive_read_next_header(h->a, &h->entry);
  if (rc == ARCHIVE_EOF)
    return api->ARGON_FALSE;

  if (rc != ARCHIVE_OK && rc != ARCHIVE_WARN) {
    return api->ARGON_FALSE;
  }

  return api->ARGON_TRUE;
})

// reader_entry_path(handle) -> string
ARGON_FUNCTION(reader_entry_path, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  const char *name = archive_entry_pathname(h->entry);
  if (!name)
    return api->ARGON_NULL;

  return api->string_to_argon((struct string){(char *)name, strlen(name)});
})

// reader_entry_size(handle) -> int
ARGON_FUNCTION(reader_entry_size, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  int64_t size = archive_entry_size_is_set(h->entry)
                     ? (int64_t)archive_entry_size(h->entry)
                     : -1;

  return api->i64_to_argon(size);
})

// reader_entry_is_dir(handle) -> bool
ARGON_FUNCTION(reader_entry_is_dir, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  return archive_entry_filetype(h->entry) == AE_IFDIR ? api->ARGON_TRUE
                                                       : api->ARGON_FALSE;
})

// reader_entry_mtime(handle) -> int
ARGON_FUNCTION(reader_entry_mtime, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  int64_t mtime = archive_entry_mtime_is_set(h->entry)
                      ? (int64_t)archive_entry_mtime(h->entry)
                      : 0;

  return api->i64_to_argon(mtime);
})

// reader_entry_mode(handle) -> int
ARGON_FUNCTION(reader_entry_mode, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  return api->i64_to_argon((int64_t)archive_entry_mode(h->entry));
})

// reader_read_data(handle, ArchiveError) -> string
// Reads the *entire* remaining data of the current entry into one
// string. Fine for the sizes stdlib users will realistically hit;
// callers wanting streaming should use reader_read_chunk in a loop.
ARGON_FUNCTION(reader_read_data, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[1];

  int64_t declared = archive_entry_size_is_set(h->entry)
                         ? (int64_t)archive_entry_size(h->entry)
                         : 0;

  size_t cap = declared > 0 ? (size_t)declared : 65536;
  size_t used = 0;
  char *buf = malloc(cap);
  if (!buf)
    return api->throw_argon_error(err, err_type, "Out of memory");

  for (;;) {
    if (used == cap) {
      size_t newcap = cap * 2;
      char *nbuf = realloc(buf, newcap);
      if (!nbuf) {
        free(buf);
        return api->throw_argon_error(err, err_type, "Out of memory");
      }
      buf = nbuf;
      cap = newcap;
    }

    ssize_t n = archive_read_data(h->a, buf + used, cap - used);
    if (n < 0) {
      ArgonObject *res =
          throw_archive_error(api, err, err_type, h->a, "Failed to read entry data");
      free(buf);
      return res;
    }
    if (n == 0)
      break;

    used += (size_t)n;
  }

  ArgonObject *result = api->string_to_argon((struct string){buf, used});
  free(buf);
  return result;
})

// reader_read_chunk(handle, ArchiveError, max_len) -> string|null
// Reads up to max_len bytes from the current entry. Returns null at
// end of entry data.
ARGON_FUNCTION(reader_read_chunk, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[1];

  int64_t max_len = api->argon_to_i64(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  if (max_len <= 0)
    max_len = 65536;

  char *buf = malloc((size_t)max_len);
  if (!buf)
    return api->throw_argon_error(err, err_type, "Out of memory");

  ssize_t n = archive_read_data(h->a, buf, (size_t)max_len);
  if (n < 0) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, h->a, "Failed to read entry data");
    free(buf);
    return res;
  }
  if (n == 0) {
    free(buf);
    return api->ARGON_NULL;
  }

  ArgonObject *result = api->string_to_argon((struct string){buf, (size_t)n});
  free(buf);
  return result;
})

// reader_extract_to(handle, dest_path, ArchiveError) -> null
// Extracts the current entry's data straight to disk at dest_path.
ARGON_FUNCTION(reader_extract_to, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h || !h->entry)
    return api->ARGON_NULL;

  struct string dest = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[2];

  char *cdest = malloc(dest.length + 1);
  if (!cdest)
    return api->throw_argon_error(err, err_type, "Out of memory");
  memcpy(cdest, dest.data, dest.length);
  cdest[dest.length] = '\0';

  // Retarget the entry's pathname so archive_write_disk writes it
  // wherever the caller asked, not wherever the archive says.
  archive_entry_set_pathname(h->entry, cdest);

  struct archive *disk = archive_write_disk_new();
  archive_write_disk_set_options(disk, ARCHIVE_EXTRACT_TIME |
                                          ARCHIVE_EXTRACT_PERM |
                                          ARCHIVE_EXTRACT_ACL |
                                          ARCHIVE_EXTRACT_FFLAGS);

  int rc = archive_write_header(disk, h->entry);
  if (rc != ARCHIVE_OK && rc != ARCHIVE_WARN) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, disk, "Failed to write entry header");
    archive_write_free(disk);
    free(cdest);
    return res;
  }

  const void *buf;
  size_t size;
  int64_t offset;
  for (;;) {
    int r = archive_read_data_block(h->a, &buf, &size, &offset);
    if (r == ARCHIVE_EOF)
      break;
    if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
      ArgonObject *res = throw_archive_error(api, err, err_type, h->a,
                                             "Failed to read entry data");
      archive_write_free(disk);
      free(cdest);
      return res;
    }

    r = (int)archive_write_data_block(disk, buf, size, offset);
    if (r < ARCHIVE_OK) {
      ArgonObject *res = throw_archive_error(api, err, err_type, disk,
                                             "Failed to write entry data");
      archive_write_free(disk);
      free(cdest);
      return res;
    }
  }

  archive_write_close(disk);
  archive_write_free(disk);
  free(cdest);
  return api->ARGON_NULL;
})

// reader_close(handle) -> null
ARGON_FUNCTION(reader_close, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h)
    return api->ARGON_NULL;

  if (!h->closed) {
    archive_read_close(h->a);
    archive_read_free(h->a);
    h->closed = 1;
  }
  free(h);
  return api->ARGON_NULL;
})

// ------------------------------------------------------------
// Writer
// ------------------------------------------------------------

// writer_open(path, format, filter, ArchiveError) -> handle
// format: "tar" | "zip" | "cpio" | "7zip" | "iso9660"
// filter: "none" | "gzip" | "bzip2" | "xz" | "zstd"
ARGON_FUNCTION(writer_open, {
  struct string path = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct string format = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct string filter = api->argon_to_string(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[3];

  struct archive *a = archive_write_new();

  int rc = ARCHIVE_OK;

  if (format.length == 3 && strncmp(format.data, "tar", 3) == 0) {
    rc = archive_write_set_format_pax_restricted(a);
  } else if (format.length == 3 && strncmp(format.data, "zip", 3) == 0) {
    rc = archive_write_set_format_zip(a);
  } else if (format.length == 4 && strncmp(format.data, "cpio", 4) == 0) {
    rc = archive_write_set_format_cpio(a);
  } else if (format.length == 4 && strncmp(format.data, "7zip", 4) == 0) {
    rc = archive_write_set_format_7zip(a);
  } else if (format.length == 7 && strncmp(format.data, "iso9660", 7) == 0) {
    rc = archive_write_set_format_iso9660(a);
  } else {
    archive_write_free(a);
    return api->throw_argon_error(err, err_type, "Unknown archive format");
  }

  if (rc != ARCHIVE_OK) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, a, "Failed to set archive format");
    archive_write_free(a);
    return res;
  }

  if (filter.length == 4 && strncmp(filter.data, "gzip", 4) == 0) {
    rc = archive_write_add_filter_gzip(a);
  } else if (filter.length == 5 && strncmp(filter.data, "bzip2", 5) == 0) {
    rc = archive_write_add_filter_bzip2(a);
  } else if (filter.length == 2 && strncmp(filter.data, "xz", 2) == 0) {
    rc = archive_write_add_filter_xz(a);
  } else if (filter.length == 4 && strncmp(filter.data, "zstd", 4) == 0) {
    rc = archive_write_add_filter_zstd(a);
  } else if (filter.length == 4 && strncmp(filter.data, "none", 4) == 0) {
    rc = archive_write_add_filter_none(a);
  } else {
    archive_write_free(a);
    return api->throw_argon_error(err, err_type, "Unknown archive filter");
  }

  if (rc != ARCHIVE_OK) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, a, "Failed to set archive filter");
    archive_write_free(a);
    return res;
  }

  char *cpath = malloc(path.length + 1);
  if (!cpath) {
    archive_write_free(a);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  memcpy(cpath, path.data, path.length);
  cpath[path.length] = '\0';

  rc = archive_write_open_filename(a, cpath);
  free(cpath);

  if (rc != ARCHIVE_OK) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, a, "Failed to open archive for writing");
    archive_write_free(a);
    return res;
  }

  ar_handle *h = malloc(sizeof(ar_handle));
  if (!h) {
    archive_write_free(a);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  h->a = a;
  h->entry = NULL;
  h->closed = 0;

  return wrap_handle(api, err, h);
})

// writer_add_file(handle, archive_path, source_path, ArchiveError) -> null
// Adds a single regular file from disk at source_path, stored in the
// archive under archive_path.
ARGON_FUNCTION(writer_add_file, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h)
    return api->ARGON_NULL;

  struct string arpath = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct string srcpath = api->argon_to_string(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[3];

  char *csrc = malloc(srcpath.length + 1);
  if (!csrc)
    return api->throw_argon_error(err, err_type, "Out of memory");
  memcpy(csrc, srcpath.data, srcpath.length);
  csrc[srcpath.length] = '\0';

  FILE *fp = fopen(csrc, "rb");
  if (!fp) {
    ArgonObject *res = api->throw_argon_error(
        err, err_type, "Failed to open source file: %s", csrc);
    free(csrc);
    return res;
  }

  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (fsize < 0)
    fsize = 0;

  struct archive_entry *entry = archive_entry_new();

  char *carpath = malloc(arpath.length + 1);
  if (!carpath) {
    archive_entry_free(entry);
    fclose(fp);
    free(csrc);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  memcpy(carpath, arpath.data, arpath.length);
  carpath[arpath.length] = '\0';

  archive_entry_set_pathname(entry, carpath);
  archive_entry_set_size(entry, fsize);
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);

  int rc = archive_write_header(h->a, entry);
  if (rc != ARCHIVE_OK && rc != ARCHIVE_WARN) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, h->a, "Failed to write entry header");
    archive_entry_free(entry);
    fclose(fp);
    free(csrc);
    free(carpath);
    return res;
  }

  char buf[65536];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
    if (archive_write_data(h->a, buf, n) < 0) {
      ArgonObject *res = throw_archive_error(api, err, err_type, h->a,
                                             "Failed to write entry data");
      archive_entry_free(entry);
      fclose(fp);
      free(csrc);
      free(carpath);
      return res;
    }
  }

  archive_entry_free(entry);
  fclose(fp);
  free(csrc);
  free(carpath);
  return api->ARGON_NULL;
})

// writer_add_bytes(handle, archive_path, data, ArchiveError) -> null
// Adds a single regular file whose content is the given in-memory
// byte string, without touching disk.
ARGON_FUNCTION(writer_add_bytes, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h)
    return api->ARGON_NULL;

  struct string arpath = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct string data = api->argon_to_string(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[3];

  struct archive_entry *entry = archive_entry_new();

  char *carpath = malloc(arpath.length + 1);
  if (!carpath) {
    archive_entry_free(entry);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  memcpy(carpath, arpath.data, arpath.length);
  carpath[arpath.length] = '\0';

  archive_entry_set_pathname(entry, carpath);
  archive_entry_set_size(entry, (int64_t)data.length);
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);

  int rc = archive_write_header(h->a, entry);
  if (rc != ARCHIVE_OK && rc != ARCHIVE_WARN) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, h->a, "Failed to write entry header");
    archive_entry_free(entry);
    free(carpath);
    return res;
  }

  if (data.length > 0 &&
      archive_write_data(h->a, data.data, data.length) < 0) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, h->a, "Failed to write entry data");
    archive_entry_free(entry);
    free(carpath);
    return res;
  }

  archive_entry_free(entry);
  free(carpath);
  return api->ARGON_NULL;
})

// writer_add_dir(handle, archive_path, ArchiveError) -> null
// Adds an empty directory entry.
ARGON_FUNCTION(writer_add_dir, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h)
    return api->ARGON_NULL;

  struct string arpath = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *err_type = argv[2];

  struct archive_entry *entry = archive_entry_new();

  char *carpath = malloc(arpath.length + 1);
  if (!carpath) {
    archive_entry_free(entry);
    return api->throw_argon_error(err, err_type, "Out of memory");
  }
  memcpy(carpath, arpath.data, arpath.length);
  carpath[arpath.length] = '\0';

  archive_entry_set_pathname(entry, carpath);
  archive_entry_set_filetype(entry, AE_IFDIR);
  archive_entry_set_perm(entry, 0755);
  archive_entry_set_size(entry, 0);

  int rc = archive_write_header(h->a, entry);
  if (rc != ARCHIVE_OK && rc != ARCHIVE_WARN) {
    ArgonObject *res =
        throw_archive_error(api, err, err_type, h->a, "Failed to write entry header");
    archive_entry_free(entry);
    free(carpath);
    return res;
  }

  archive_entry_free(entry);
  free(carpath);
  return api->ARGON_NULL;
})

// writer_close(handle) -> null
ARGON_FUNCTION(writer_close, {
  ar_handle *h = get_handle(argv[0], err, api);
  if (!h)
    return api->ARGON_NULL;

  if (!h->closed) {
    archive_write_close(h->a);
    archive_write_free(h->a);
    h->closed = 1;
  }
  free(h);
  return api->ARGON_NULL;
})

INIT_ARGON_MODULE({
  REGISTER_ARGON_FUNCTION(reader_open);
  REGISTER_ARGON_FUNCTION(reader_open_memory);
  REGISTER_ARGON_FUNCTION(reader_next);
  REGISTER_ARGON_FUNCTION(reader_entry_path);
  REGISTER_ARGON_FUNCTION(reader_entry_size);
  REGISTER_ARGON_FUNCTION(reader_entry_is_dir);
  REGISTER_ARGON_FUNCTION(reader_entry_mtime);
  REGISTER_ARGON_FUNCTION(reader_entry_mode);
  REGISTER_ARGON_FUNCTION(reader_read_data);
  REGISTER_ARGON_FUNCTION(reader_read_chunk);
  REGISTER_ARGON_FUNCTION(reader_extract_to);
  REGISTER_ARGON_FUNCTION(reader_close);

  REGISTER_ARGON_FUNCTION(writer_open);
  REGISTER_ARGON_FUNCTION(writer_add_file);
  REGISTER_ARGON_FUNCTION(writer_add_bytes);
  REGISTER_ARGON_FUNCTION(writer_add_dir);
  REGISTER_ARGON_FUNCTION(writer_close);
})
