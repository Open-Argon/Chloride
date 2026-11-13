// SPDX-FileCopyrightText: 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifdef _WIN32
#define timegm _mkgmtime
#define WIN_PTHREADS_TIME_H // block pthread_time.h's clock_gettime declaration
#include <windows.h>
#endif

#define _GNU_SOURCE
#include "Argon.h"
#include "ArgonFunction.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include "win32_strptime.h"
typedef int clockid_t;
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

static int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  if (clk_id == CLOCK_MONOTONIC) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    tp->tv_sec = count.QuadPart / freq.QuadPart;
    tp->tv_nsec =
        (long)((count.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart);
  } else {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long t =
        ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    t -= 116444736000000000ULL;
    tp->tv_sec = t / 10000000;
    tp->tv_nsec = (long)((t % 10000000) * 100);
  }
  return 0;
}
#endif

// Thread-safe conversions from a Unix timestamp to calendar fields.
static bool utc_parts(time_t t, struct tm *out) {
#ifdef _WIN32
  return gmtime_s(out, &t) == 0;
#else
  return gmtime_r(&t, out) != NULL;
#endif
}

static bool local_parts(time_t t, struct tm *out) {
#ifdef _WIN32
  return localtime_s(out, &t) == 0;
#else
  return localtime_r(&t, out) != NULL;
#endif
}

static ArgonObject *parts_to_argon(struct tm *tm, ArgonError *err,
                                   ArgonNativeAPI *api) {
  ArgonObject *parts = api->create_argon_buffer(sizeof(struct tm));
  struct tm *buf = api->argon_buffer_to_buffer(parts, err).data;
  if (api->is_error(err))
    return api->ARGON_NULL;
  *buf = *tm;
  return parts;
}

static ArgonObject *format_parts(struct tm *tm, ArgonObject *format,
                                 ArgonError *err, ArgonNativeAPI *api) {
  struct string fmt_str = api->argon_to_string(format, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  char *fmt = malloc(fmt_str.length + 1);
  if (!fmt)
    return api->ARGON_NULL;
  memcpy(fmt, fmt_str.data, fmt_str.length);
  fmt[fmt_str.length] = '\0';

  // strftime returns 0 both for an empty result and for a buffer that is too
  // small, so grow the buffer a few times before giving up.
  size_t capacity = 256;
  char *buf = NULL;
  size_t length = 0;
  for (int attempt = 0; attempt < 6; attempt++) {
    char *tmp = realloc(buf, capacity);
    if (!tmp)
      break;
    buf = tmp;
    length = strftime(buf, capacity, fmt, tm);
    if (length > 0 || fmt_str.length == 0)
      break;
    capacity *= 4;
  }
  free(fmt);
  ArgonObject *result = api->string_to_argon((struct string){buf, length});
  free(buf);
  return result;
}

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

ARGON_FUNCTION(time_monotonic, {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return api->i64_to_argon((int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec);
})

ARGON_FUNCTION(time_to_parts_utc, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct tm tm;
  if (!utc_parts((time_t)seconds, &tm))
    return api->ARGON_NULL;
  return parts_to_argon(&tm, err, api);
})

ARGON_FUNCTION(time_to_parts_local, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct tm tm;
  if (!local_parts((time_t)seconds, &tm))
    return api->ARGON_NULL;
  return parts_to_argon(&tm, err, api);
})

// The local time zone's offset from UTC, in seconds, at the given instant
// (positive east of Greenwich, and including daylight saving time).
ARGON_FUNCTION(time_utc_offset, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct tm tm;
  if (!local_parts((time_t)seconds, &tm))
    return api->i64_to_argon(0);
  tm.tm_isdst = 0;
  return api->i64_to_argon((int64_t)timegm(&tm) - seconds);
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
ARGON_FUNCTION(time_parts_get_wday,
               { GET_tm return api->i64_to_argon(tm->tm_wday); }) // 0=Sunday
ARGON_FUNCTION(time_parts_get_yday,
               { GET_tm return api->i64_to_argon(tm->tm_yday + 1); }) // 1-365

ARGON_FUNCTION(time_from_parts, {
  struct tm t = {0};
  t.tm_year = (int)api->argon_to_i64(argv[0], err) - 1900;
  t.tm_mon = (int)api->argon_to_i64(argv[1], err) - 1;
  t.tm_mday = (int)api->argon_to_i64(argv[2], err);
  t.tm_hour = (int)api->argon_to_i64(argv[3], err);
  t.tm_min = (int)api->argon_to_i64(argv[4], err);
  t.tm_sec = (int)api->argon_to_i64(argv[5], err);
  t.tm_isdst = -1;
  return api->i64_to_argon((int64_t)mktime(&t));
})

ARGON_FUNCTION(time_from_parts_utc, {
  struct tm t = {0};
  t.tm_year = (int)api->argon_to_i64(argv[0], err) - 1900;
  t.tm_mon = (int)api->argon_to_i64(argv[1], err) - 1;
  t.tm_mday = (int)api->argon_to_i64(argv[2], err);
  t.tm_hour = (int)api->argon_to_i64(argv[3], err);
  t.tm_min = (int)api->argon_to_i64(argv[4], err);
  t.tm_sec = (int)api->argon_to_i64(argv[5], err);
  t.tm_isdst = 0;
  return api->i64_to_argon((int64_t)timegm(&t));
})

ARGON_FUNCTION(time_format_utc, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct tm tm;
  if (!utc_parts((time_t)seconds, &tm))
    return api->ARGON_NULL;
  return format_parts(&tm, argv[1], err, api);
})

ARGON_FUNCTION(time_format_local, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;
  int64_t seconds = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct tm tm;
  if (!local_parts((time_t)seconds, &tm))
    return api->ARGON_NULL;
  return format_parts(&tm, argv[1], err, api);
})

// Parses a string with a strptime format. Returns the parsed fields, or null
// if the string doesn't match the format (including leftover characters).
// Fields missing from the format default to 1970-01-01 00:00:00.
ARGON_FUNCTION(time_parse, {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;
  struct tm t = {0};
  t.tm_year = 70;
  t.tm_mday = 1;
  t.tm_isdst = -1;
  struct string str_str = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  char *str = malloc(str_str.length + 1);
  if (!str)
    return api->ARGON_NULL;
  memcpy(str, str_str.data, str_str.length);
  str[str_str.length] = '\0';

  struct string fmt_str = api->argon_to_string(argv[1], err);
  if (api->is_error(err)) {
    free(str);
    return api->ARGON_NULL;
  }
  char *fmt = malloc(fmt_str.length + 1);
  if (!fmt) {
    free(str);
    return api->ARGON_NULL;
  }
  memcpy(fmt, fmt_str.data, fmt_str.length);
  fmt[fmt_str.length] = '\0';

  char *rest = strptime(str, fmt, &t);
  bool matched = rest != NULL && *rest == '\0';
  free(str);
  free(fmt);
  if (!matched)
    return api->ARGON_NULL;

  return parts_to_argon(&t, err, api);
})

INIT_ARGON_MODULE({
  REGISTER_ARGON_FUNCTION(time_now);
  REGISTER_ARGON_FUNCTION(time_get_seconds);
  REGISTER_ARGON_FUNCTION(time_get_subsecond);
  REGISTER_ARGON_FUNCTION(time_monotonic);
  REGISTER_ARGON_FUNCTION(time_to_parts_utc);
  REGISTER_ARGON_FUNCTION(time_to_parts_local);
  REGISTER_ARGON_FUNCTION(time_utc_offset);
  REGISTER_ARGON_FUNCTION(time_parts_get_year);
  REGISTER_ARGON_FUNCTION(time_parts_get_month);
  REGISTER_ARGON_FUNCTION(time_parts_get_day);
  REGISTER_ARGON_FUNCTION(time_parts_get_hour);
  REGISTER_ARGON_FUNCTION(time_parts_get_minute);
  REGISTER_ARGON_FUNCTION(time_parts_get_second);
  REGISTER_ARGON_FUNCTION(time_parts_get_wday);
  REGISTER_ARGON_FUNCTION(time_parts_get_yday);
  REGISTER_ARGON_FUNCTION(time_from_parts);
  REGISTER_ARGON_FUNCTION(time_from_parts_utc);
  REGISTER_ARGON_FUNCTION(time_format_utc);
  REGISTER_ARGON_FUNCTION(time_format_local);
  REGISTER_ARGON_FUNCTION(time_parse);
})