/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef Argon_NATIVE_H
#define Argon_NATIVE_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif

#include <stdint.h>
#include <stdbool.h>

#define ARGON_NATIVE_API_VERSION 1

typedef struct ArgonState ArgonState;

typedef struct ArgonError ArgonError;

typedef struct ArgonObjectRegister ArgonObjectRegister;

typedef struct ArgonObject ArgonObject;

typedef struct ArgonNativeAPI ArgonNativeAPI;

typedef ArgonObject *(*native_fn)(size_t argc, ArgonObject **argv,
                                  ArgonError *err, ArgonState *state,
                                  ArgonNativeAPI *api);

struct number {
  int64_t n;
  int64_t d;
};

struct ArgonNativeAPI {
  void (*register_ArgonObject)(ArgonObjectRegister *reg, char *name,
                               ArgonObject *obj);
  ArgonObject *(*create_argon_native_function)(char *name, native_fn);
  ArgonObject *(*throw_argon_error)(ArgonError *err, const char *type,
                                    const char *fmt, ...);
  bool (*is_error)(ArgonError *err);

  ArgonObject *(*i64_to_argon)(int64_t);
  ArgonObject *(*double_to_argon)(double);
  ArgonObject *(*num_and_den_to_argon)(int64_t n, uint64_t d);

  int64_t (*argon_to_i64)(ArgonObject *, ArgonError *);
  double (*argon_to_double)(ArgonObject *, ArgonError *);
  struct number (*argon_to_num_and_den)(ArgonObject *, ArgonError *);

  ArgonObject *ARGON_NULL;
  ArgonObject *ARGON_TRUE;
  ArgonObject *ARGON_FALSE;
  // void (*argon_register_function)(ArgonObject *, const char *, native_fn,
  // int);
};

__attribute__((visibility("default"))) void
argon_module_init(ArgonState *vm, ArgonNativeAPI *api,
                  ArgonObjectRegister *reg);

#ifdef __cplusplus
}
#endif

#endif // Argon_NATIVE_H