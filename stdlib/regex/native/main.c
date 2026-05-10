// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// Helper: extract pcre2_code* from an ArgonObject buffer
static pcre2_code *get_re(ArgonObject *obj, ArErr*err, ArgonNativeAPI *api) {
  struct buffer buf = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err))
    return NULL;
  return *(pcre2_code **)buf.data;
}

// Helper: create an ArgonObject string from a substring of a C string
static ArgonObject *make_string(ArgonNativeAPI *api, const char *data, PCRE2_SIZE len) {
  return api->string_to_argon((struct string){(char *)data, len});
}

// compile(pattern, RegexError) -> buffer<pcre2_code*>
ARGON_FUNCTION(compile, {
  pcre2_code *re;
  int errcode;
  PCRE2_SIZE erroffset;

  struct string str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  re = pcre2_compile((PCRE2_SPTR)str.data, str.length, PCRE2_UTF | PCRE2_UCP,
                     &errcode, &erroffset, NULL);

  if (!re) {
    PCRE2_UCHAR errbuf[256];
    pcre2_get_error_message(errcode, errbuf, sizeof(errbuf));
    ArgonObject *err_type = argv[1];
    return api->throw_argon_error(err, err_type,
                                  "Regex compile error at offset %d: %s\n",
                                  (int)erroffset, errbuf);
  }

  pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);

  ArgonObject *re_ptr_obj = api->create_argon_buffer(sizeof(pcre2_code *));
  struct buffer re_ptr_buf = api->argon_buffer_to_buffer(re_ptr_obj, err);
  if (api->is_error(err)) {
    pcre2_code_free(re);
    return api->ARGON_NULL;
  }

  *(pcre2_code **)re_ptr_buf.data = re;
  return re_ptr_obj;
})

// free(re_buf) -> null
ARGON_FUNCTION(re_free, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  pcre2_code_free(re);
  return api->ARGON_NULL;
})

// match(re_buf, subject) -> bool
ARGON_FUNCTION(match, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  struct string subject = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
  int rc = pcre2_match(re, (PCRE2_SPTR)subject.data, subject.length, 0, 0, md, NULL);
  pcre2_match_data_free(md);

  return rc >= 0?api->ARGON_TRUE:api->ARGON_FALSE;
})

// find(re_buf, subject, append_fn, list) -> list
// append_fn is called with each capture group string, index 0 is full match
ARGON_FUNCTION(find, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  struct string subject = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  // argv[2] = append_fn, argv[3] = list to append into
  ArgonObject *append_fn = argv[2];
  ArgonObject *list      = argv[3];

  pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
  int rc = pcre2_match(re, (PCRE2_SPTR)subject.data, subject.length, 0, 0, md, NULL);

  if (rc < 0) {
    pcre2_match_data_free(md);
    return api->ARGON_NULL;
  }

  PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);

  for (int i = 0; i < rc; i++) {
    ArgonObject *s = make_string(api, subject.data + ov[2*i], ov[2*i+1] - ov[2*i]);
    api->call(append_fn, 1, (ArgonObject *[]){s}, NULL, err, state);
    if (api->is_error(err)) {
      pcre2_match_data_free(md);
      return api->ARGON_NULL;
    }
  }

  pcre2_match_data_free(md);
  return list;
})

// find_all(re_buf, subject, append_fn, outer_list, inner_append_fn, inner_list_factory) -> outer_list
// For each match, calls inner_list_factory() to get a new list, appends captures into it,
// then appends that inner list into outer_list via append_fn
ARGON_FUNCTION(find_all, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  struct string subject = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *append_fn      = argv[2]; // outer list append
  ArgonObject *outer_list     = argv[3];
  ArgonObject *inner_append   = argv[4]; // inner list append
  ArgonObject *list_factory   = argv[5]; // callable -> new empty list

  pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
  PCRE2_SIZE offset = 0;
  int rc;

  while ((rc = pcre2_match(re, (PCRE2_SPTR)subject.data, subject.length,
                           offset, 0, md, NULL)) >= 0) {
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);

    // create a new inner list for this match's captures
    ArgonObject *inner_list = api->call(list_factory, 0, NULL, NULL, err, state);
    if (api->is_error(err)) {
      pcre2_match_data_free(md);
      return api->ARGON_NULL;
    }

    for (int i = 0; i < rc; i++) {
      ArgonObject *s = make_string(api, subject.data + ov[2*i], ov[2*i+1] - ov[2*i]);
      api->call(inner_append, 1, (ArgonObject *[]){s}, NULL, err, state);
      if (api->is_error(err)) {
        pcre2_match_data_free(md);
        return api->ARGON_NULL;
      }
    }

    api->call(append_fn, 1, (ArgonObject *[]){inner_list}, NULL, err, state);
    if (api->is_error(err)) {
      pcre2_match_data_free(md);
      return api->ARGON_NULL;
    }

    offset = ov[1];
    if (ov[0] == ov[1]) offset++;
  }

  pcre2_match_data_free(md);
  return outer_list;
})

// replace(re_buf, subject, replacement) -> string
ARGON_FUNCTION(replace, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  struct string subject     = api->argon_to_string(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  struct string replacement = api->argon_to_string(argv[2], err);
  if (api->is_error(err)) return api->ARGON_NULL;

  PCRE2_SIZE outlen = subject.length * 2 + replacement.length + 1;
  char *outbuf = malloc(outlen);
  if (!outbuf) return api->ARGON_NULL;

  int rc = pcre2_substitute(
      re,
      (PCRE2_SPTR)subject.data, subject.length,
      0, 0, NULL, NULL,
      (PCRE2_SPTR)replacement.data, replacement.length,
      (PCRE2_UCHAR *)outbuf, &outlen
  );

  if (rc < 0) {
    free(outbuf);
    return api->ARGON_NULL;
  }

  ArgonObject *result = make_string(api, outbuf, outlen);
  free(outbuf);
  return result;
})

// replace_all(re_buf, subject, replacement) -> string
ARGON_FUNCTION(replace_all, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  struct string subject     = api->argon_to_string(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  struct string replacement = api->argon_to_string(argv[2], err);
  if (api->is_error(err)) return api->ARGON_NULL;

  PCRE2_SIZE outlen = subject.length * 2 + replacement.length + 1;
  char *outbuf = malloc(outlen);
  if (!outbuf) return api->ARGON_NULL;

  int rc = pcre2_substitute(
      re,
      (PCRE2_SPTR)subject.data, subject.length,
      0,
      PCRE2_SUBSTITUTE_GLOBAL,
      NULL, NULL,
      (PCRE2_SPTR)replacement.data, replacement.length,
      (PCRE2_UCHAR *)outbuf, &outlen
  );

  if (rc < 0) {
    free(outbuf);
    return api->ARGON_NULL;
  }

  ArgonObject *result = make_string(api, outbuf, outlen);
  free(outbuf);
  return result;
})

// split(re_buf, subject, append_fn, list) -> list
ARGON_FUNCTION(split, {
  pcre2_code *re = get_re(argv[0], err, api);
  if (!re)
    return api->ARGON_NULL;

  struct string subject = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *append_fn = argv[2];
  ArgonObject *list      = argv[3];

  pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
  PCRE2_SIZE offset = 0;
  int rc;

  while ((rc = pcre2_match(re, (PCRE2_SPTR)subject.data, subject.length,
                           offset, 0, md, NULL)) >= 0) {
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);

    ArgonObject *s = make_string(api, subject.data + offset, ov[0] - offset);
    api->call(append_fn, 1, (ArgonObject *[]){s}, NULL, err, state);
    if (api->is_error(err)) {
      pcre2_match_data_free(md);
      return api->ARGON_NULL;
    }

    offset = ov[1];
    if (ov[0] == ov[1]) offset++;
  }

  // remainder after last match
  ArgonObject *tail = make_string(api, subject.data + offset, subject.length - offset);
  api->call(append_fn, 1, (ArgonObject *[]){tail}, NULL, err, state);

  pcre2_match_data_free(md);
  return list;
})

INIT_ARGON_MODULE({
  REGISTER_ARGON_FUNCTION(compile)
  REGISTER_ARGON_FUNCTION(re_free)
  REGISTER_ARGON_FUNCTION(match)
  REGISTER_ARGON_FUNCTION(find)
  REGISTER_ARGON_FUNCTION(find_all)
  REGISTER_ARGON_FUNCTION(replace)
  REGISTER_ARGON_FUNCTION(replace_all)
  REGISTER_ARGON_FUNCTION(split)
})