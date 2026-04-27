/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../../call/call.h"
#include "../array/array.h"
#include "../exceptions/exceptions.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../object.h"
#include "../slice/slice.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *ARGON_STRING_TYPE = NULL;

ArgonObject *ARGON_STRING_ITERATOR_TYPE = NULL;
/* Returns the number of bytes consumed, or 0 on invalid UTF-8.
   Writes the decoded codepoint to *cp. */
static int decode_utf8(const unsigned char *s, size_t len, uint32_t *cp) {
  if (len == 0)
    return 0;
  if (s[0] < 0x80) {
    *cp = s[0];
    return 1;
  } else if ((s[0] & 0xE0) == 0xC0 && len >= 2) {
    *cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    return 2;
  } else if ((s[0] & 0xF0) == 0xE0 && len >= 3) {
    *cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    return 3;
  } else if ((s[0] & 0xF8) == 0xF0 && len >= 4) {
    *cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) |
          (s[3] & 0x3F);
    return 4;
  }
  return 0; /* invalid */
}

char *c_quote_string(const char *input, size_t len) {
  // Worst case: every byte is a 4-byte UTF-8 sequence needing a surrogate pair
  // Each surrogate is "\uXXXX" (6 chars), so 12 chars per 4 bytes, plus quotes
  // + NUL
  size_t max_out = 2 + (len * 12) + 1;
  char *out = malloc(max_out);
  if (!out)
    return NULL;

  size_t j = 0;
  out[j++] = '"';

  for (size_t i = 0; i < len;) {
    unsigned char c = (unsigned char)input[i];

    switch (c) {
    case '\n':
      out[j++] = '\\';
      out[j++] = 'n';
      i++;
      break;
    case '\t':
      out[j++] = '\\';
      out[j++] = 't';
      i++;
      break;
    case '\r':
      out[j++] = '\\';
      out[j++] = 'r';
      i++;
      break;
    case '\\':
      out[j++] = '\\';
      out[j++] = '\\';
      i++;
      break;
    case '\"':
      out[j++] = '\\';
      out[j++] = '"';
      i++;
      break;
    default:
      if (isprint(c) && c < 0x80) {
        out[j++] = c;
        i++;
      } else {
        uint32_t cp;
        int consumed =
            decode_utf8((const unsigned char *)input + i, len - i, &cp);
        if (consumed <= 0) {
          // Invalid UTF-8 byte: escape it raw and skip
          j += sprintf(&out[j], "\\u%04X", c);
          i++;
        } else if (cp <= 0xFFFF) {
          j += sprintf(&out[j], "\\u%04X", cp);
          i += consumed;
        } else {
          // Encode as a UTF-16 surrogate pair
          cp -= 0x10000;
          uint32_t hi = 0xD800 + (cp >> 10);
          uint32_t lo = 0xDC00 + (cp & 0x3FF);
          j += sprintf(&out[j], "\\u%04X\\u%04X", hi, lo);
          i += consumed;
        }
      }
      break;
    }
  }

  out[j++] = '"';
  out[j] = '\0';
  return out;
}

ARGON_METHOD(ARGON_STRING_TYPE, __equal__, {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  return (argv[0]->type == TYPE_STRING && argv[1]->type == TYPE_STRING) &&
                 (!argv[0]->value.as_str->hash ||
                  !argv[1]->value.as_str->hash ||
                  argv[0]->value.as_str->hash == argv[1]->value.as_str->hash) &&
                 (argv[0]->value.as_str->length ==
                  argv[1]->value.as_str->length) &&
                 (memcmp(argv[0]->value.as_str->data,
                         argv[1]->value.as_str->data,
                         argv[0]->value.as_str->length) == 0)
             ? ARGON_TRUE
             : ARGON_FALSE;
})

ARGON_METHOD(ARGON_STRING_TYPE, __not_equal__, {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__not_equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return ARGON_STRING_TYPE___equal__(argc, argv, err, state, api) == ARGON_TRUE
             ? ARGON_FALSE
             : ARGON_TRUE;
})

ARGON_METHOD(ARGON_STRING_TYPE, __less_than__, {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__less_than__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err(RuntimeError, "__less_than__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp < 0)
    return ARGON_TRUE;
  if (cmp > 0)
    return ARGON_FALSE;

  return len1 < len2 ? ARGON_TRUE : ARGON_FALSE;
})

ARGON_METHOD(ARGON_STRING_TYPE, __less_than_equal__, {
  (void)api;
  (void)state;

  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__less_than_equal__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err(RuntimeError, "__less_than_equal__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp < 0)
    return ARGON_TRUE;
  if (cmp > 0)
    return ARGON_FALSE;

  return len1 <= len2 ? ARGON_TRUE : ARGON_FALSE;
})

ARGON_METHOD(ARGON_STRING_TYPE, __greater_than__, {
  (void)api;
  (void)state;

  if (argc != 2) {
    *err =
        create_err(RuntimeError,
                   "__greater_than__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err(RuntimeError, "__greater_than__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp > 0)
    return ARGON_TRUE;
  if (cmp < 0)
    return ARGON_FALSE;

  return len1 > len2 ? ARGON_TRUE : ARGON_FALSE;
})

ARGON_METHOD(ARGON_STRING_TYPE, __greater_than_equal__, {
  (void)api;
  (void)state;

  if (argc != 2) {
    *err = create_err(
        RuntimeError,
        "__greater_than_equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  if (argv[0]->type != TYPE_STRING || argv[1]->type != TYPE_STRING) {
    *err = create_err(RuntimeError, "__greater_than_equal__ expects strings");
    return ARGON_NULL;
  }

  size_t len1 = argv[0]->value.as_str->length;
  size_t len2 = argv[1]->value.as_str->length;

  size_t min_len = len1 < len2 ? len1 : len2;

  int cmp =
      memcmp(argv[0]->value.as_str->data, argv[1]->value.as_str->data, min_len);

  if (cmp > 0)
    return ARGON_TRUE;
  if (cmp < 0)
    return ARGON_FALSE;

  return len1 >= len2 ? ARGON_TRUE : ARGON_FALSE;
})

ARGON_METHOD(ARGON_STRING_TYPE, get_length, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "get_length expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0]->value.as_str->length);
})

ARGON_METHOD(ARGON_STRING_TYPE, set_length, {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "set_length expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  *err = create_err(RuntimeError, "attribute 'length' is immutable");
  return ARGON_NULL;
})

ARGON_METHOD(ARGON_STRING_TYPE, upper, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "upper expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  ArgonObject *new_str_obj = api->string_to_argon(self);
  struct string new_str = api->argon_to_string(new_str_obj, err);
  if (api->is_error(err))
    return ARGON_NULL;

  for (size_t i = 0; i < new_str.length; i++) {
    new_str.data[i] = toupper((unsigned char)new_str.data[i]);
  }

  return new_str_obj;
})

ARGON_METHOD(ARGON_STRING_TYPE, lower, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "lower expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  ArgonObject *new_str_obj = api->string_to_argon(self);
  struct string new_str = api->argon_to_string(new_str_obj, err);
  if (api->is_error(err))
    return ARGON_NULL;

  for (size_t i = 0; i < new_str.length; i++) {
    new_str.data[i] = tolower((unsigned char)new_str.data[i]);
  }

  return new_str_obj;
})

ARGON_METHOD(ARGON_STRING_TYPE, title, {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "upper expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  ArgonObject *new_str_obj = api->string_to_argon(self);
  struct string new_str = api->argon_to_string(new_str_obj, err);
  if (api->is_error(err))
    return ARGON_NULL;

  bool new_word = true;
  for (size_t i = 0; i < new_str.length; i++) {
    if (new_str.data[i] == ' ') {
      new_word = true;
      continue;
    }
    if (new_word) {
      new_str.data[i] = toupper((unsigned char)new_str.data[i]);
      new_word = false; // always reset, not just when char changed
    } else {
      new_str.data[i] = tolower((unsigned char)new_str.data[i]);
    }
  }

  return new_str_obj;
})

ARGON_METHOD(ARGON_STRING_TYPE, replace, {
  (void)api;
  (void)state;
  if (argc != 3) {
    *err = create_err(RuntimeError, "replace expects 3 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  struct string old = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;

  struct string new = api->argon_to_string(argv[2], err);
  if (api->is_error(err))
    return ARGON_NULL;

  // edge case: replacing empty string is undefined, just return self
  if (old.length == 0)
    return argv[0];

  // count occurrences to pre-calculate the result length
  size_t count = 0;
  for (size_t i = 0; i + old.length <= self.length;) {
    if (memcmp(self.data + i, old.data, old.length) == 0) {
      count++;
      i += old.length;
    } else {
      i++;
    }
  }

  if (count == 0)
    return argv[0];

  size_t new_len = self.length + count * (new.length - old.length);
  char *buf = malloc(new_len + 1);
  if (!buf) {
    *err = create_err(RuntimeError, "out of memory");
    return ARGON_NULL;
  }

  // build the result
  size_t src = 0, dst = 0;
  while (src + old.length <= self.length) {
    if (memcmp(self.data + src, old.data, old.length) == 0) {
      memcpy(buf + dst, new.data, new.length);
      dst += new.length;
      src += old.length;
    } else {
      buf[dst++] = self.data[src++];
    }
  }
  // copy any remaining tail
  memcpy(buf + dst, self.data + src, self.length - src);
  buf[new_len] = '\0';

  struct string result = {.data = buf, .length = new_len};
  ArgonObject *result_obj = api->string_to_argon(result);
  free(buf);
  return result_obj;
})

ARGON_METHOD(ARGON_STRING_TYPE, __getitem__, {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__getitem__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  if (argv[1]->type == TYPE_SLICE) { // slice

    SliceIndices indices;
    if (slice_indices(argv[1], self.length, &indices, err, api) != 0)
      return ARGON_NULL;

    int64_t size = 0;
    if (indices.step > 0 && indices.stop > indices.start)
      size = (indices.stop - indices.start + indices.step - 1) / indices.step;
    else if (indices.step < 0 && indices.stop < indices.start)
      size =
          (indices.start - indices.stop - indices.step - 1) / (-indices.step);

    char *slice = checked_malloc(size);

    for (int64_t i = 0; i < size; i++) {
      int64_t src_index =
          indices.start + i * indices.step; // source index follows step
      slice[i] = self.data[src_index];
    }
    ArgonObject *result = api->string_to_argon((struct string){slice, size});
    free(slice);
    return result;
  }
  int64_t index = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (index < 0)
    index += self.length;
  if (index >= (int64_t)self.length || index < 0) {
    return api->throw_argon_error(err, IndexError, "index out of range");
  }
  return api->string_to_argon((struct string){&self.data[index], 1});
})
ARGON_METHOD(ARGON_STRING_TYPE, split, {
  if (argc != 2) {
    *err = create_err(RuntimeError, "split expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  struct string delim = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;

  size_t dlen = delim.length;

  if (dlen == 0) {
    return api->throw_argon_error(err, ValueError, "empty separator");
  }

  ArgonObject *object = new_instance(ARRAY_TYPE, sizeof(darray_armem));
  object->type = TYPE_ARRAY;
  object->value.as_array = darray_armem_create();
  darray_armem_init(object->value.as_array, sizeof(ArgonObject *), 0);

  // Guard: if source string is empty or has no data, return array with
  // just the original (empty) string as the sole element.
  if (s.length == 0 || s.data == NULL) {
    ArgonObject *item = api->string_to_argon((struct string){s.data, 0});
    darray_armem_insert(object->value.as_array, object->value.as_array->size,
                        &item);
    return object;
  }

  // Guard: delimiter data must not be null (length already checked above).
  if (delim.data == NULL) {
    return api->throw_argon_error(err, ValueError, "invalid separator");
  }

  char *start = s.data;
  char *end   = s.data + s.length;
  char *p     = start;

  while (p <= end - dlen) {
    if (memcmp(p, delim.data, dlen) == 0) {
      ArgonObject *item =
          api->string_to_argon((struct string){start, p - start});
      darray_armem_insert(object->value.as_array, object->value.as_array->size,
                          &item);
      p += dlen;
      start = p;
      continue;
    }
    p++;
  }

  // Final segment
  ArgonObject *item =
      api->string_to_argon((struct string){start, end - start});
  darray_armem_insert(object->value.as_array, object->value.as_array->size,
                      &item);

  return object;
})

ARGON_METHOD(ARGON_STRING_TYPE, strip, {
  if (argc != 2) {
    *err = create_err(RuntimeError, "strip expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  struct string chars = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;

  // Empty string or no strip chars: return a copy as-is
  if (s.length == 0 || s.data == NULL || chars.length == 0 || chars.data == NULL) {
    return api->string_to_argon((struct string){s.data, s.length});
  }

  char *start = s.data;
  char *end   = s.data + s.length - 1; // points to last char

  // Strip from the left
  while (start <= end && memchr(chars.data, (unsigned char)*start, chars.length) != NULL) {
    start++;
  }

  // Strip from the right
  while (end >= start && memchr(chars.data, (unsigned char)*end, chars.length) != NULL) {
    end--;
  }

  // end is now pointing at the last kept char, so length = end - start + 1
  return api->string_to_argon((struct string){start, end - start + 1});
})

ARGON_METHOD(ARGON_STRING_TYPE, __contains__, {
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__contains__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;
  if (argv[1]->type != TYPE_STRING)
    return ARGON_FALSE;
  struct string slice = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return ARGON_NULL;

  if (self.length < slice.length)
    return ARGON_FALSE;

  // Empty needle is always contained
  if (slice.length == 0)
    return ARGON_TRUE;

  // Boyer-Moore-Horspool: build bad character skip table
  size_t skip[256];
  for (size_t i = 0; i < 256; i++)
    skip[i] = slice.length;
  for (size_t i = 0; i < slice.length - 1; i++)
    skip[(unsigned char)slice.data[i]] = slice.length - 1 - i;

  // Search
  size_t i = slice.length - 1;
  while (i < self.length) {
    size_t j = slice.length - 1;
    size_t k = i;
    while (j < slice.length && self.data[k] == slice.data[j]) {
      if (j == 0)
        return ARGON_TRUE;
      k--;
      j--;
    }
    i += skip[(unsigned char)self.data[i]];
  }

  return ARGON_FALSE;
})

char *char_chr(uint64_t codepoint, size_t *len_out) {
  char *out;

  // Unicode only allows up to 0x10FFFF
  if (codepoint > 0x10FFFF) {
    if (len_out)
      *len_out = 0;
    return NULL;
  }

  out = malloc(4);
  if (!out) {
    if (len_out)
      *len_out = 0;
    return NULL;
  }

  if (codepoint <= 0x7F) {
    out[0] = (char)codepoint;
    if (len_out)
      *len_out = 1;
  } else if (codepoint <= 0x7FF) {
    out[0] = 0xC0 | (codepoint >> 6);
    out[1] = 0x80 | (codepoint & 0x3F);
    if (len_out)
      *len_out = 2;
  } else if (codepoint <= 0xFFFF) {
    out[0] = 0xE0 | (codepoint >> 12);
    out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
    out[2] = 0x80 | (codepoint & 0x3F);
    if (len_out)
      *len_out = 3;
  } else {
    out[0] = 0xF0 | (codepoint >> 18);
    out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
    out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
    out[3] = 0x80 | (codepoint & 0x3F);
    if (len_out)
      *len_out = 4;
  }

  return out;
}

uint64_t char_ord(const char *s, size_t len, bool *valid) {
  *valid = false;
  if (len == 0 || len > 4)
    return 0;

  const unsigned char *u = (const unsigned char *)s;
  uint64_t codepoint = 0;

  if (u[0] < 0x80 && len == 1) {
    *valid = true;
    return u[0];
  }

  // 2-byte sequence
  if ((u[0] >> 5) == 0x6 && len == 2) {
    codepoint = ((u[0] & 0x1F) << 6) | (u[1] & 0x3F);
    *valid = true;
    return codepoint;
  }

  // 3-byte sequence
  if ((u[0] >> 4) == 0xE && len == 3) {
    codepoint = ((u[0] & 0x0F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
    *valid = true;
    return codepoint;
  }

  // 4-byte sequence
  if ((u[0] >> 3) == 0x1E && len == 4) {
    codepoint = ((u[0] & 0x07) << 18) | ((u[1] & 0x3F) << 12) |
                ((u[2] & 0x3F) << 6) | (u[3] & 0x3F);
    *valid = true;
    return codepoint;
  }

  return 0; // invalid UTF-8
}

ARGON_METHOD(ARGON_STRING_TYPE, chr, {
  (void)state;
  if (argc != 1) {
    *err =
        create_err(RuntimeError, "chr expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  int64_t codepoint = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;
  size_t length;
  char *data = char_chr(codepoint, &length);
  if (!data)
    return api->throw_argon_error(err, ValueError,
                                  "codepoint %" PRIu64
                                  " is not a valid unicode codepoint");
  ArgonObject *result = api->string_to_argon((struct string){data, length});
  free(data);
  return result;
})

ARGON_METHOD(ARGON_STRING_TYPE, ord, {
  (void)state;
  if (argc != 1) {
    *err =
        create_err(RuntimeError, "chr expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  struct string str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;
  bool valid;
  int64_t codepoint = char_ord(str.data, str.length, &valid);
  if (!valid)
    return api->throw_argon_error(err, ValueError, "invalid unicode character");
  return api->i64_to_argon(codepoint);
})

struct {
  struct string_struct as_str;
  char chr;
  ArgonObject obj;
} small_chars[CHAR_MAX - CHAR_MIN + 1];

struct {
  struct string_struct as_str;
  ArgonObject obj;
} empty_str;

void init_small_chars() {
  empty_str.obj.type = TYPE_STRING;
  empty_str.obj.dict = NULL;
  empty_str.obj.value.as_str = &empty_str.as_str;
  add_builtin_field(&empty_str.obj, __class__, ARGON_STRING_TYPE);
  empty_str.obj.value.as_str->data = NULL;
  empty_str.obj.value.as_str->length = 0;
  empty_str.obj.as_bool = false;

  for (int64_t i = 0; i <= CHAR_MAX - CHAR_MIN; i++) {
    int64_t n = i + CHAR_MIN;
    small_chars[i].obj.type = TYPE_STRING;
    small_chars[i].obj.dict = NULL;
    small_chars[i].obj.value.as_str = &small_chars[i].as_str;
    add_builtin_field(&small_chars[i].obj, __class__, ARGON_STRING_TYPE);
    small_chars[i].chr = n;
    small_chars[i].obj.value.as_str->data = &small_chars[i].chr;
    small_chars[i].obj.value.as_str->length = 1;
    small_chars[i].obj.as_bool = true;
  }
}

void init_string(ArgonObject *object, char *data, size_t length,
                 uint64_t hash) {
  object->type = TYPE_STRING;
  object->value.as_str =
      (struct string_struct *)((char *)object + sizeof(ArgonObject));
  object->value.as_str->data = data;
  object->value.as_str->hash = hash;
  object->value.as_str->length = length;
  object->as_bool = length;
}

ArgonObject *new_string_object_without_memcpy(char *data, size_t length,
                                              uint64_t hash) {
  ArgonObject *object =
      new_small_instance(ARGON_STRING_TYPE, sizeof(struct string_struct));
  init_string(object, data, length, hash);
  return object;
}

ArgonObject *new_string_object(char *data, size_t length, uint64_t hash) {
  if (length == 0)
    return &empty_str.obj;
  if (length == 1)
    return &small_chars[data[0] - CHAR_MIN].obj;
  char *data_copy = ar_alloc_atomic(length);
  memcpy(data_copy, data, length);
  return new_string_object_without_memcpy(data_copy, length, hash);
}

char *argon_object_to_length_terminated_string_from___string__(
    ArgonObject *object, ArErr *err, RuntimeState *state, size_t *length) {
  ArgonObject *string_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __string__, object);

  if (!string_convert_method) {
    *length = sizeof("<object>") - 1;
    return "<object>";
  }

  ArgonObject *string_object =
      argon_call(string_convert_method, 0, NULL, err, state);
  if (is_error(err)) {
    *length = 0;
    return "";
  }

  if (string_object->type != TYPE_STRING) {
    *length = sizeof("<object>") - 1;
    return "<object>";
  }

  *length = string_object->value.as_str->length;
  return string_object->value.as_str->data;
}

char *argon_object_to_null_terminated_string(ArgonObject *object, ArErr *err,
                                             RuntimeState *state) {
  ArgonObject *string_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __repr__, object);

  if (!string_convert_method)
    return "<object>";

  ArgonObject *string_object =
      argon_call(string_convert_method, 0, NULL, err, state);
  if (is_error(err))
    return "";

  if (string_object->type != TYPE_STRING)
    return "<object>";

  char *string = ar_alloc(string_object->value.as_str->length + 1);
  string[string_object->value.as_str->length] = '\0';
  memcpy(string, string_object->value.as_str->data,
         string_object->value.as_str->length);
  return string;
}

char *argon_string_to_c_string_malloc(ArgonObject *object) {
  if (object->type != TYPE_STRING)
    return NULL;

  char *string = malloc(object->value.as_str->length + 1);
  string[object->value.as_str->length] = '\0';
  memcpy(string, object->value.as_str->data, object->value.as_str->length);
  return string;
}

ArgonObject *new_string_object_null_terminated(char *data) {
  return new_string_object(data, strlen(data), 0);
}

ARGON_METHOD(ARGON_STRING_TYPE, __iter__, {
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }

  struct string self = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return ARGON_NULL;

  ArgonObject *iterator = new_instance(ARGON_STRING_ITERATOR_TYPE,
                                       sizeof(struct as_string_iterator));
  iterator->type = TYPE_STRING_ITERATOR;
  iterator->value.as_string_iterator =
      (struct as_string_iterator *)((char *)iterator + sizeof(ArgonObject));
  iterator->value.as_string_iterator->current = 0;
  iterator->value.as_string_iterator->data = self.data;
  iterator->value.as_string_iterator->length = self.length;
  return iterator;
})

ARGON_METHOD(ARGON_STRING_ITERATOR_TYPE, __next__, {
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__next__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  if (self->type != TYPE_STRING_ITERATOR)
    return api->throw_argon_error(err, TypeError, "expected string iterator");
  struct as_string_iterator *string_iterator = self->value.as_string_iterator;

  if (string_iterator->current >= string_iterator->length) {
    err->ptr = StopIteration_instance;
    return ARGON_NULL;
  }

  return api->string_to_argon(
      (struct string){&string_iterator->data[string_iterator->current++], 1});
})

ArgonObject *ARGON_RENDER_TEMPLATE;

ArgonObject *RENDER_TEMPLATE(size_t argc, ArgonObject **argv, ArErr *err,
                             RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 1) {
    *err = create_err(RuntimeError,
                      "RENDER_TEMPLATE expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  size_t capacity = 128;
  char *string = checked_malloc(capacity);
  size_t string_length = 0;

  struct tuple_struct *tuple = &argv[0]->value.as_tuple;

  for (size_t i = 0; i < tuple->size; i++) {
    ArgonObject *item = tuple->data[i]->value.as_tuple.data[1];

    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(item, __class__), __string__, item);

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, err, state);
      bool resized = false;
      while (capacity < string_length + string_object->value.as_str->length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_object->value.as_str->data,
             string_object->value.as_str->length);
      string_length += string_object->value.as_str->length;
    } else {
      char *string_obj = "<object>";
      size_t length = strlen(string_obj);
      bool resized = false;
      while (capacity < string_length + length) {
        capacity *= 2;
        resized = true;
      }
      if (resized)
        string = realloc(string, capacity);
      memcpy(string + string_length, string_obj, length);
      string_length += length;
    }
  }
  ArgonObject *result = new_string_object(string, string_length, 0);

  free(string);
  return result;
}