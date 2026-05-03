// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <stddef.h>
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include "Argon.h"
#include "ArgonFunction.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

ARGON_FUNCTION(time_now, {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;
  ArgonObject *ts_obj = api->create_argon_buffer(sizeof(struct timespec));
  struct timespec *ts = api->argon_buffer_to_buffer(ts_obj, err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;

  clock_gettime(CLOCK_REALTIME, ts);

  return ts_obj;
})

ARGON_FUNCTION(time_get_seconds, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  struct timespec *ts = api->argon_buffer_to_buffer(argv[0], err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->i64_to_argon(ts->tv_sec);
})

ARGON_FUNCTION(time_get_subsecond, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  struct timespec *ts = api->argon_buffer_to_buffer(argv[0], err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->i64_to_argon(ts->tv_nsec);
})

ARGON_FUNCTION(time_to_parts_local, {
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  time_t t = (time_t)seconds;
  struct tm *tm = localtime(&t);

  ArgonObject *parts = api->create_argon_buffer(sizeof(struct tm));
  struct tm *buf = api->argon_buffer_to_buffer(parts, err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;

  *buf = *tm;
  return parts;
})

#define GET_tm                                                                 \
  if (api->fix_to_arg_size(1, argc, err))                                      \
    return api->ARGON_NULL;                                                    \
  struct tm *tm = api->argon_buffer_to_buffer(argv[0], err).data;

ARGON_FUNCTION(time_parts_get_year,
               { GET_tm return api->i64_to_argon(tm->tm_year + 1900); })
ARGON_FUNCTION(time_parts_get_month,
               { GET_tm return api->i64_to_argon(tm->tm_mon + 1); })
ARGON_FUNCTION(time_parts_get_day,
               { GET_tm return api->i64_to_argon(tm->tm_mday); })
ARGON_FUNCTION(time_parts_get_hour,
               { GET_tm return api->i64_to_argon(tm->tm_hour); })
ARGON_FUNCTION(time_parts_get_minute,
               { GET_tm return api->i64_to_argon(tm->tm_min); })
ARGON_FUNCTION(time_parts_get_second,
               { GET_tm return api->i64_to_argon(tm->tm_sec); })

ARGON_FUNCTION(time_from_parts, {
  struct tm t = {0};
  t.tm_year = (int)api->argon_to_i64(argv[0], err) - 1900;
  t.tm_mon = (int)api->argon_to_i64(argv[1], err) - 1;
  t.tm_mday = (int)api->argon_to_i64(argv[2], err);
  t.tm_hour = (int)api->argon_to_i64(argv[3], err);
  t.tm_min = (int)api->argon_to_i64(argv[4], err);
  t.tm_sec = (int)api->argon_to_i64(argv[5], err);
  t.tm_isdst = -1; // let the OS figure out DST
  return api->i64_to_argon((int64_t)mktime(&t));
})

ARGON_FUNCTION(time_format, {
  // argv[0] = parts buffer, argv[1] = format string
  struct tm t = {0};
  t.tm_year = (int)api->argon_to_i64(argv[0], err) - 1900;
  t.tm_mon = (int)api->argon_to_i64(argv[1], err) - 1;
  t.tm_mday = (int)api->argon_to_i64(argv[2], err);
  t.tm_hour = (int)api->argon_to_i64(argv[3], err);
  t.tm_min = (int)api->argon_to_i64(argv[4], err);
  t.tm_sec = (int)api->argon_to_i64(argv[5], err);
  struct string fmt_str = api->argon_to_string(argv[6], err);

  char *fmt = malloc(fmt_str.length + 1);

  if (!fmt)
    return api->ARGON_NULL;

  memcpy(fmt, fmt_str.data, fmt_str.length);
  fmt[fmt_str.length] = '\0';

  char buf[256];
  size_t length = strftime(buf, sizeof(buf), fmt, &t);
  free(fmt);
  return api->string_to_argon((struct string){buf, length});
})

ARGON_FUNCTION(time_parse, {
  struct tm t = {0};
  struct string str_str = api->argon_to_string(argv[0], err);  // was argv[6]
  char *str = malloc(str_str.length + 1);
  if (!str)                                                      // was if (str)
    return api->ARGON_NULL;
  memcpy(str, str_str.data, str_str.length);
  str[str_str.length] = '\0';

  struct string fmt_str = api->argon_to_string(argv[1], err);  // was argv[6]
  char *fmt = malloc(fmt_str.length + 1);
  if (!fmt)                                                      // was if (fmt)
    return api->ARGON_NULL;
  memcpy(fmt, fmt_str.data, fmt_str.length);
  fmt[fmt_str.length] = '\0';

  strptime(str, fmt, &t);
  free(str);
  free(fmt);

  ArgonObject *parts = api->create_argon_buffer(sizeof(struct tm));
  struct tm *buf = api->argon_buffer_to_buffer(parts, err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;
  *buf = t;
  return parts;
})

ARGON_FUNCTION(time_to_parts_utc, {
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  time_t t = (time_t)seconds;
  struct tm *tm = gmtime(&t);

  ArgonObject *parts = api->create_argon_buffer(sizeof(struct tm));
  struct tm *buf = api->argon_buffer_to_buffer(parts, err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;

  *buf = *tm;
  return parts;
})

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;
  REGISTER_ARGON_FUNCTION(time_now)
  REGISTER_ARGON_FUNCTION(time_get_seconds)
  REGISTER_ARGON_FUNCTION(time_get_subsecond)
  REGISTER_ARGON_FUNCTION(time_to_parts_local)
  REGISTER_ARGON_FUNCTION(time_parts_get_year)
  REGISTER_ARGON_FUNCTION(time_parts_get_month)
  REGISTER_ARGON_FUNCTION(time_parts_get_day)
  REGISTER_ARGON_FUNCTION(time_parts_get_hour)
  REGISTER_ARGON_FUNCTION(time_parts_get_minute)
  REGISTER_ARGON_FUNCTION(time_parts_get_second)
  REGISTER_ARGON_FUNCTION(time_from_parts)
  REGISTER_ARGON_FUNCTION(time_format)
  REGISTER_ARGON_FUNCTION(time_parse)
  REGISTER_ARGON_FUNCTION(time_to_parts_utc)
}