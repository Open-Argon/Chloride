/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "runtime.h"
#include "../err.h"
#include "../hash_data/hash_data.h"
#include "../memory.h"
#include "../parser/number/number.h"
#include "../translator/translator.h"
#include "api/api.h"
#include "assignment/assignment.h"
#include "call/call.h"
#include "declaration/declaration.h"
#include "import/import.h"
#include "internals/dynamic_array_armem/darray_armem.h"
#include "internals/hashmap/hashmap.h"
#include "native_loader/native_loader.h"
#include "objects/array/array.h"
#include "objects/buffer/buffer.h"
#include "objects/dictionary/dictionary.h"
#include "objects/functions/functions.h"
#include "objects/literals/literals.h"
#include "objects/number/number.h"
#include "objects/object.h"
#include "objects/string/string.h"
#include "objects/term/term.h"
#include "objects/tuple/tuple.h"
#include "objects/type/type.h"
#include <fcntl.h>
#include <gc/gc.h>
#include <gmp.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(_WIN32)
#include <windows.h>
#else
extern char **environ;
#endif

#if defined(_WIN32) || defined(_WIN64)

#define PLATFORM_OS "windows"
#define PLATFORM_LIB_PREFIX ""
#define PLATFORM_LIB_EXT ".dll"

#elif defined(__APPLE__) && defined(__MACH__)

#define PLATFORM_OS "darwin"
#define PLATFORM_LIB_PREFIX "lib"
#define PLATFORM_LIB_EXT ".dylib"

#elif defined(__linux__)

#define PLATFORM_OS "linux"
#define PLATFORM_LIB_PREFIX "lib"
#define PLATFORM_LIB_EXT ".so"

#elif defined(__FreeBSD__)

#define PLATFORM_OS "freebsd"
#define PLATFORM_LIB_PREFIX "lib"
#define PLATFORM_LIB_EXT ".so"

#elif defined(__NetBSD__)

#define PLATFORM_OS "netbsd"
#define PLATFORM_LIB_PREFIX "lib"
#define PLATFORM_LIB_EXT ".so"

#elif defined(__OpenBSD__)

#define PLATFORM_OS "openbsd"
#define PLATFORM_LIB_PREFIX "lib"
#define PLATFORM_LIB_EXT ".so"

#elif defined(__unix__)

#define PLATFORM_OS "unix"
#define PLATFORM_LIB_PREFIX "lib"
#define PLATFORM_LIB_EXT ".so"

#else

#define PLATFORM_OS "unknown"
#define PLATFORM_LIB_PREFIX ""
#define PLATFORM_LIB_EXT ""

#endif

#define POP_BYTE() bc[ip++]
#define POP_U64(dst)                                                           \
  do {                                                                         \
    memcpy(&(dst), bc + ip, 8);                                                \
    ip += 8;                                                                   \
  } while (0)

ArgonObject *ARGON_METHOD_TYPE;
Stack *Global_Scope = NULL;
ArgonObject *ADDITION_FUNCTION;
ArgonObject *SUBTRACTION_FUNCTION;
ArgonObject *MULTIPLY_FUNCTION;
ArgonObject *EXPONENT_FUNCTION;
ArgonObject *DIVIDE_FUNCTION;
ArgonObject *FLOOR_DIVIDE_FUNCTION;
ArgonObject *MODULO_FUNCTION;
ArgonObject *EQUAL_FUNCTION;
ArgonObject *NOT_EQUAL_FUNCTION;
ArgonObject *LESS_THAN_FUNCTION;
ArgonObject *LESS_THAN_EQUAL_FUNCTION;
ArgonObject *GREATER_THAN_FUNCTION;
ArgonObject *GREATER_THAN_EQUAL_FUNCTION;

ArgonObject *BASE_CLASS___getattribute__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)state;
  (void)api;
  if (argc != 2) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__getattribute__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *to_access = argv[0];
  ArgonObject *access = argv[1];
  uint64_t hash;
  if (access->value.as_str->hash_computed) {
    hash = access->value.as_str->hash;
  } else {
    hash =
        runtime_hash(access->value.as_str->data, access->value.as_str->length,
                     access->value.as_str->prehash);
    access->value.as_str->hash = hash;
    access->value.as_str->hash_computed = true;
  }
  char *name_text = access->value.as_str->data;
  size_t name_len = access->value.as_str->length;

  ArgonObject *value =
      get_field_l(to_access, name_text, hash, name_len, true, false);

  if (value)
    return value;

  size_t getter_len = name_len + 4; // "get_" = 4
  char *getter_name = malloc(getter_len);

  memcpy(getter_name, "get_", 4);
  memcpy(getter_name + 4, name_text, name_len);

  /* If your get_field_l needs a hash, recompute it */
  uint64_t getter_hash = runtime_hash(getter_name, getter_len, 0);

  ArgonObject *class_to_access = get_builtin_field(to_access, __class__);

  ArgonObject *getter = get_field_for_class_l(
      class_to_access, getter_name, getter_hash, getter_len, to_access);
  free(getter_name);
  if (getter)
    return argon_call(getter, 0, (ArgonObject *[]){}, err, state);

  ArgonObject *cls__getattr__ =
      get_builtin_field_for_class(class_to_access, __getattr__, to_access);
  if (cls__getattr__) {
    value =
        argon_call(cls__getattr__, 1, (ArgonObject *[]){access}, err, state);
    if (err->exists) {
      return ARGON_NULL;
    }
    return value;
  }
  ArgonObject *name =
      get_builtin_field_for_class(class_to_access, __name__, to_access);
  *err = create_err(
      0, 0, 0, "", "Runtime Error", "'%.*s' object has no attribute '%.*s'",
      (int)name->value.as_str->length, name->value.as_str->data,
      (int)access->value.as_str->length, access->value.as_str->data);
  return ARGON_NULL;
}

ArgonObject *ARGON_ADDITION_FUNCTION(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "add expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *object__add__ =
        get_builtin_field_for_class(object_class, __add__, output);
    if (!object__add__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __add__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output =
        argon_call(object__add__, 1, (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_SUBTRACTION_FUNCTION(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "subtract expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function__subtract__ =
        get_builtin_field_for_class(object_class, __subtract__, output);
    if (!function__subtract__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __subtract__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function__subtract__, 1, (ArgonObject *[]){argv[i]},
                        err, state);
  }
  return output;
}

ArgonObject *ARGON_MULTIPLY_FUNCTION(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "multiply expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function__multiply__ =
        get_builtin_field_for_class(object_class, __multiply__, output);
    if (!function__multiply__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __multiply__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function__multiply__, 1, (ArgonObject *[]){argv[i]},
                        err, state);
  }
  return output;
}

ArgonObject *ARGON_EXPONENT_FUNCTION(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "multiply expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function__multiply__ =
        get_builtin_field_for_class(object_class, __multiply__, output);
    if (!function__multiply__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __multiply__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function__multiply__, 1, (ArgonObject *[]){argv[i]},
                        err, state);
  }
  return output;
}

ArgonObject *ARGON_DIVIDE_FUNCTION(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "divide expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___divide__ =
        get_builtin_field_for_class(object_class, __division__, output);
    if (!function___divide__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __divide__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___divide__, 1, (ArgonObject *[]){argv[i]}, err,
                        state);
  }
  return output;
}

ArgonObject *ARGON_FLOOR_DIVIDE_FUNCTION(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "floor_divide expects at least 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___floor_divide__ =
        get_builtin_field_for_class(object_class, __floor_division__, output);
    if (!function___floor_divide__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err =
          create_err(0, 0, 0, "", "Runtime Error",
                     "Object of type '%.*s' is missing __floor_divide__ method",
                     (int)cls___name__->value.as_str->length,
                     cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___floor_divide__, 1,
                        (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_MODULO_FUNCTION(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "modulo expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___modulo__ =
        get_builtin_field_for_class(object_class, __modulo__, output);
    if (!function___modulo__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __modulo__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___modulo__, 1, (ArgonObject *[]){argv[i]}, err,
                        state);
  }
  return output;
}

ArgonObject *ARGON_EQUAL_FUNCTION(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "equal expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___equal__ =
        get_builtin_field_for_class(object_class, __equal__, output);
    if (!function___equal__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __equal__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___equal__, 1, (ArgonObject *[]){argv[i]}, err,
                        state);
  }
  return output;
}

ArgonObject *ARGON_NOT_EQUAL_FUNCTION(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "not_equal expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___not_equal__ =
        get_builtin_field_for_class(object_class, __not_equal__, output);
    if (!function___not_equal__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __not_equal__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___not_equal__, 1, (ArgonObject *[]){argv[i]},
                        err, state);
  }
  return output;
}

ArgonObject *ARGON_LESS_THAN_FUNCTION(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "less_than expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___less_than__ =
        get_builtin_field_for_class(object_class, __less_than__, output);
    if (!function___less_than__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object of type '%.*s' is missing __less_than__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___less_than__, 1, (ArgonObject *[]){argv[i]},
                        err, state);
  }
  return output;
}

ArgonObject *ARGON_GREATER_THAN_FUNCTION(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "greater_than expects at least 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___greater_than__ =
        get_builtin_field_for_class(object_class, __greater_than__, output);
    if (!function___greater_than__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err =
          create_err(0, 0, 0, "", "Runtime Error",
                     "Object of type '%.*s' is missing __greater_than__ method",
                     (int)cls___name__->value.as_str->length,
                     cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___greater_than__, 1,
                        (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_LESS_THAN_EQUAL_FUNCTION(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state,
                                            ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "less_than_equal expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___less_than_equal__ =
        get_builtin_field_for_class(object_class, __less_than_equal__, output);
    if (!function___less_than_equal__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(
          0, 0, 0, "", "Runtime Error",
          "Object of type '%.*s' is missing __less_than_equal__ method",
          (int)cls___name__->value.as_str->length,
          cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___less_than_equal__, 1,
                        (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_GREATER_THAN_EQUAL_FUNCTION(size_t argc, ArgonObject **argv,
                                               ArErr *err, RuntimeState *state,
                                               ArgonNativeAPI *api) {
  (void)api;
  if (argc < 1) {
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "greater_than_equal expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object_class = get_builtin_field(output, __class__);
    ArgonObject *function___greater_than_equal__ = get_builtin_field_for_class(
        object_class, __greater_than_equal__, output);
    if (!function___greater_than_equal__) {
      ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
      *err = create_err(
          0, 0, 0, "", "Runtime Error",
          "Object of type '%.*s' is missing __greater_than_equal__ method",
          (int)cls___name__->value.as_str->length,
          cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___greater_than_equal__, 1,
                        (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_TYPE_TYPE___call__(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__call__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *cls = argv[0];
  if (cls == ARGON_TYPE_TYPE && argc == 2) {
    ArgonObject *cls_class = get_builtin_field(argv[1], __class__);
    if (cls_class)
      return cls_class;
    return ARGON_NULL;
  }
  ArgonObject *cls___new__ =
      get_builtin_field_for_class(argv[0], __new__, NULL);
  if (!cls___new__) {
    ArgonObject *cls___name__ = get_builtin_field(argv[0], __name__);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "Object '%.*s' is missing __new__ method, so cannot be initialised",
        (int)cls___name__->value.as_str->length,
        cls___name__->value.as_str->data);
    return ARGON_NULL;
  }

  ArgonObject *new_object = argon_call(cls___new__, argc, argv, err, state);
  if (err->exists)
    return ARGON_NULL;
  ArgonObject *ARGON_TYPE_TYPE___call___args[] = {ARGON_TYPE_TYPE, new_object};
  ArgonObject *new_object_class = ARGON_TYPE_TYPE___call__(
      sizeof(ARGON_TYPE_TYPE___call___args) / sizeof(ArgonObject *),
      ARGON_TYPE_TYPE___call___args, err, state, &native_api);
  if (new_object_class != ARGON_NULL && new_object_class == cls) {
    ArgonObject *cls___init__ =
        get_builtin_field_for_class(argv[0], __init__, new_object);
    if (!cls___init__) {
      ArgonObject *cls___name__ = get_builtin_field(argv[0], __name__);
      *err = create_err(
          0, 0, 0, "", "Runtime Error",
          "Object '%.*s' is missing __init__ method, so cannot be initialised",
          (int)cls___name__->value.as_str->length,
          cls___name__->value.as_str->data);
    }
    argon_call(cls___init__, argc - 1, argv + 1, err, state);
    if (err->exists)
      return ARGON_NULL;
  }

  return new_object;
}

ArgonObject *BASE_CLASS_address(size_t argc, ArgonObject **argv, ArErr *err,
                                RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%p", argv[0]);
  return new_string_object_null_terminated(buffer);
}

ArgonObject *BASE_CLASS___new__(size_t argc, ArgonObject **argv, ArErr *err,
                                RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj = new_instance(argv[0], 0);
  return new_obj;
}

ArgonObject *BASE_CLASS___init__(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  (void)argv;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__init__ expects 1 argument, got %" PRIu64, argc);
  }
  return ARGON_NULL;
}

ArgonObject *BASE_CLASS___setattr__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__setattr__ expects 3 argument, got %" PRIu64, argc);
  }

  char *name_text = argv[1]->value.as_str->data;
  size_t name_len = argv[1]->value.as_str->length;

  size_t setter_len = name_len + 4; // "get_" = 4
  char *setter_name = malloc(setter_len);

  memcpy(setter_name, "set_", 4);
  memcpy(setter_name + 4, name_text, name_len);

  /* If your get_field_l needs a hash, recompute it */
  uint64_t setter_hash = runtime_hash(setter_name, setter_len, 0);

  ArgonObject *setter =
      get_field_for_class_l(get_builtin_field(argv[0], __class__), setter_name,
                            setter_hash, setter_len, argv[0]);
  free(setter_name);
  if (setter)
    return argon_call(setter, 1, (ArgonObject *[]){argv[2]}, err, state);

  if (!argv[1]->value.as_str->hash)
    argv[1]->value.as_str->hash =
        runtime_hash(argv[1]->value.as_str->data, argv[1]->value.as_str->length,
                     argv[1]->value.as_str->prehash);
  add_field_l(argv[0], argv[1]->value.as_str->data, argv[1]->value.as_str->hash,
              argv[1]->value.as_str->length, argv[2]);
  return argv[2];
}

ArgonObject *BASE_CLASS___string__(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 argument, got %" PRIu64, argc);
  }

  ArgonObject *object_name =
      get_builtin_field_for_class(argv[0], __name__, NULL);
  ArgonObject *class_name = get_builtin_field_for_class(
      get_builtin_field(argv[0], __class__), __name__, NULL);

  char buffer[100];
  if (class_name && object_name)
    snprintf(buffer, sizeof(buffer), "<%.*s %.*s at %p>",
             (int)class_name->value.as_str->length,
             class_name->value.as_str->data,
             (int)object_name->value.as_str->length,
             object_name->value.as_str->data, argv[0]);
  else if (class_name)
    snprintf(buffer, sizeof(buffer), "<%.*s object at %p>",
             (int)class_name->value.as_str->length,
             class_name->value.as_str->data, argv[0]);
  else
    snprintf(buffer, sizeof(buffer), "<object at %p>", argv[0]);
  return new_string_object_null_terminated(buffer);
}

ArgonObject *BASE_CLASS___repr__(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__repr__ expects 1 argument, got %" PRIu64, argc);
  }
  ArgonObject *string_method = get_builtin_field_for_class(
      get_builtin_field(argv[0], __class__), __string__, argv[0]);
  return argon_call(string_method, 0, NULL, err, state);
}

ArgonObject *BASE_CLASS___boolean__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 argument, got %" PRIu64, argc);
  }
  return ARGON_TRUE;
}

ArgonObject *BASE_CLASS___equal__(size_t argc, ArgonObject **argv, ArErr *err,
                                  RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__equal__ expects 2 arguments, got %" PRIu64, argc);
  }
  return argv[0] == argv[1] ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *BASE_CLASS___not_equal__(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state,
                                      ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 2 arguments, got %" PRIu64, argc);
  }
  return argv[0] != argv[1] ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *ARGON_STRING_TYPE___new__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state,
                                       ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj = new_instance(argv[0], sizeof(struct string_struct));
  return new_obj;
}

ArgonObject *ARGON_STRING_TYPE___init__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__init__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *object = argv[1];
  ArgonObject *string_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __string__, object);
  if (string_convert_method) {
    ArgonObject *string_object =
        argon_call(string_convert_method, 0, NULL, err, state);
    if (err->exists)
      return ARGON_NULL;
    init_string(self, string_object->value.as_str->data,
                string_object->value.as_str->length,
                string_object->value.as_str->prehash,
                string_object->value.as_str->hash);
    return ARGON_NULL;
  }
  *err = create_err(0, 0, 0, "", "String Conversion Error",
                    "cannot convert to string");
  return ARGON_NULL;
}

ArgonObject *ARGON_BOOL_TYPE___new__(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api) {
  (void)api;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__new__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *object = argv[1];

  self->type = TYPE_STRING;
  ArgonObject *boolean_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __boolean__, object);
  if (boolean_convert_method) {
    ArgonObject *boolean_object =
        argon_call(boolean_convert_method, 0, NULL, err, state);
    if (err->exists)
      return ARGON_NULL;
    return boolean_object;
  }
  ArgonObject *type_name = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __name__, object);
  *err = create_err(
      0, 0, 0, "", "Runtime Error", "cannot convert type '%.*s' to bool",
      type_name->value.as_str->length, type_name->value.as_str->data);
  return ARGON_NULL;
}

ArgonObject *ARGON_STRING_TYPE___add__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state,
                                       ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__add__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_STRING) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__add__ cannot perform concatenation between a string and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }
  size_t length = argv[0]->value.as_str->length + argv[1]->value.as_str->length;
  char *concat = ar_alloc_atomic(length);
  memcpy(concat, argv[0]->value.as_str->data, argv[0]->value.as_str->length);
  memcpy(concat + argv[0]->value.as_str->length, argv[1]->value.as_str->data,
         argv[1]->value.as_str->length);
  ArgonObject *object = new_string_object_without_memcpy(concat, length, 0, 0);
  return object;
}

ArgonObject *ARGON_BOOL_TYPE___string__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_string_object_null_terminated(argv[0] == ARGON_TRUE ? "true"
                                                                 : "false");
}

ArgonObject *ARGON_BOOL_TYPE___number__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__number__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0] == ARGON_TRUE);
}

ArgonObject *ARGON_STRING_TYPE___string__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 argument, got %" PRIu64, argc);
  }
  return argv[0];
}
ArgonObject *ARGON_STRING_TYPE___repr__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__repr__ expects 1 argument, got %" PRIu64, argc);
  }
  char *quoted = c_quote_string(argv[0]->value.as_str->data,
                                argv[0]->value.as_str->length);
  ArgonObject *result = new_string_object_null_terminated(quoted);
  free(quoted);
  return result;
}
ArgonObject *ARGON_STRING_TYPE___hash__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__hash__ expects 1 argument, got %" PRIu64, argc);
  }
  uint64_t hash;
  if (argv[0]->value.as_str->hash_computed) {
    hash = argv[0]->value.as_str->hash;
  } else {
    hash =
        runtime_hash(argv[0]->value.as_str->data, argv[0]->value.as_str->length,
                     argv[0]->value.as_str->prehash);
    argv[0]->value.as_str->hash = hash;
    argv[0]->value.as_str->hash_computed = true;
  }
  return new_number_object_from_int64(hash);
}

ArgonObject *ARGON_STRING_TYPE___number__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__number__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  mpq_t r;
  mpq_init(r);
  int result = mpq_set_decimal_str_exp(r, argv[0]->value.as_str->data,
                                       argv[0]->value.as_str->length);
  if (result != 0) {
    mpq_clear(r);
    *err = create_err(0, 0, 0, "", "Runtime Error", "Unable to parse number",
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *object = new_number_object(r);
  mpq_clear(r);
  return object;
}

ArgonObject *ARGON_STRING_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                           ArErr *err, RuntimeState *state,
                                           ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 argument, got %" PRIu64, argc);
  }
  return argv[0]->value.as_str->length == 0 ? ARGON_FALSE : ARGON_TRUE;
}

ArgonObject *ARGON_BOOL_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 argument, got %" PRIu64, argc);
  }
  return argv[0];
}

ArgonObject *ARGON_NULL_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 argument, got %" PRIu64, argc);
  }
  return ARGON_FALSE;
}
ArgonObject *ARGON_NULL_TYPE___number__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 argument, got %" PRIu64, argc);
  }
  return new_number_object_from_int64(0);
}
ArgonObject *ARGON_NULL_TYPE___string__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 argument, got %" PRIu64, argc);
  }
  return new_string_object_null_terminated("null");
}

void bootstrap_types() {
  BASE_CLASS = new_class();
  ARGON_TYPE_TYPE = new_class();
  add_builtin_field(ARGON_TYPE_TYPE, __base__, BASE_CLASS);
  add_builtin_field(ARGON_TYPE_TYPE, __class__, ARGON_TYPE_TYPE);

  ARGON_NULL_TYPE = new_class();
  add_builtin_field(ARGON_NULL_TYPE, __base__, BASE_CLASS);
  ARGON_NULL = new_instance(ARGON_NULL_TYPE, 0);
  ARGON_NULL->type = TYPE_NULL;
  ARGON_NULL->as_bool = false;

  add_builtin_field(BASE_CLASS, __base__, ARGON_NULL);
  add_builtin_field(BASE_CLASS, __class__, ARGON_TYPE_TYPE);

  ARGON_BOOL_TYPE = new_class();
  add_builtin_field(ARGON_BOOL_TYPE, __base__, BASE_CLASS);
  ARGON_TRUE = new_instance(ARGON_BOOL_TYPE, 0);
  ARGON_TRUE->type = TYPE_BOOL;
  ARGON_FALSE = new_instance(ARGON_BOOL_TYPE, 0);
  ARGON_FALSE->type = TYPE_BOOL;
  ARGON_NULL->as_bool = false;

  ARGON_STRING_TYPE = new_class();
  add_builtin_field(ARGON_STRING_TYPE, __base__, BASE_CLASS);

  add_builtin_field(ARGON_STRING_TYPE, __name__,
                    new_string_object_null_terminated("string"));
  add_builtin_field(BASE_CLASS, __name__,
                    new_string_object_null_terminated("object"));
  add_builtin_field(ARGON_TYPE_TYPE, __name__,
                    new_string_object_null_terminated("type"));
  add_builtin_field(ARGON_NULL_TYPE, __name__,
                    new_string_object_null_terminated("null_type"));
  add_builtin_field(ARGON_BOOL_TYPE, __name__,
                    new_string_object_null_terminated("boolean"));

  ARGON_FUNCTION_TYPE = new_class();
  add_builtin_field(ARGON_FUNCTION_TYPE, __base__, BASE_CLASS);
  add_builtin_field(ARGON_FUNCTION_TYPE, __name__,
                    new_string_object_null_terminated("function"));

  ARGON_METHOD_TYPE = new_class();
  add_builtin_field(ARGON_METHOD_TYPE, __base__, BASE_CLASS);
  add_builtin_field(ARGON_METHOD_TYPE, __name__,
                    new_string_object_null_terminated("method"));

  add_builtin_field(
      BASE_CLASS, __new__,
      create_argon_native_function("__new__", BASE_CLASS___new__));
  add_builtin_field(
      BASE_CLASS, field__address,
      create_argon_native_function("address", BASE_CLASS_address));
  add_builtin_field(
      BASE_CLASS, __init__,
      create_argon_native_function("__init__", BASE_CLASS___new__));
  add_builtin_field(
      BASE_CLASS, __string__,
      create_argon_native_function("__string__", BASE_CLASS___string__));
  add_builtin_field(
      BASE_CLASS, __repr__,
      create_argon_native_function("__repr__", BASE_CLASS___repr__));
  add_builtin_field(
      ARGON_TYPE_TYPE, __call__,
      create_argon_native_function("__call__", ARGON_TYPE_TYPE___call__));
  add_builtin_field(
      ARGON_STRING_TYPE, __new__,
      create_argon_native_function("__new__", ARGON_STRING_TYPE___new__));
  add_builtin_field(
      ARGON_STRING_TYPE, __init__,
      create_argon_native_function("__init__", ARGON_STRING_TYPE___init__));
  add_builtin_field(
      ARGON_STRING_TYPE, __hash__,
      create_argon_native_function("__hash__", ARGON_STRING_TYPE___hash__));
  add_builtin_field(
      ARGON_STRING_TYPE, get_length,
      create_argon_native_function("get_length", ARGON_STRING_TYPE_get_length));
  add_builtin_field(
      ARGON_STRING_TYPE, set_length,
      create_argon_native_function("set_length", ARGON_STRING_TYPE_set_length));
  add_builtin_field(
      ARGON_STRING_TYPE, __add__,
      create_argon_native_function("__add__", ARGON_STRING_TYPE___add__));
  add_builtin_field(
      ARGON_STRING_TYPE, __number__,
      create_argon_native_function("__number__", ARGON_STRING_TYPE___number__));
  add_builtin_field(
      ARGON_NULL_TYPE, __boolean__,
      create_argon_native_function("__boolean__", ARGON_NULL_TYPE___boolean__));
  add_builtin_field(
      ARGON_NULL_TYPE, __string__,
      create_argon_native_function("__string__", ARGON_NULL_TYPE___string__));
  add_builtin_field(
      ARGON_NULL_TYPE, __number__,
      create_argon_native_function("__number__", ARGON_NULL_TYPE___number__));
  add_builtin_field(
      ARGON_STRING_TYPE, __string__,
      create_argon_native_function("__string__", ARGON_STRING_TYPE___string__));
  add_builtin_field(
      ARGON_STRING_TYPE, __repr__,
      create_argon_native_function("__repr__", ARGON_STRING_TYPE___repr__));
  add_builtin_field(
      ARGON_BOOL_TYPE, __new__,
      create_argon_native_function("__new__", ARGON_BOOL_TYPE___new__));
  add_builtin_field(
      ARGON_BOOL_TYPE, __boolean__,
      create_argon_native_function("__boolean__", ARGON_BOOL_TYPE___boolean__));
  add_builtin_field(ARGON_STRING_TYPE, __boolean__,
                    create_argon_native_function(
                        "__boolean__", ARGON_STRING_TYPE___boolean__));
  add_builtin_field(
      BASE_CLASS, __boolean__,
      create_argon_native_function("__boolean__", BASE_CLASS___boolean__));
  add_builtin_field(
      BASE_CLASS, __equal__,
      create_argon_native_function("__equal__", BASE_CLASS___equal__));
  add_builtin_field(
      ARGON_BOOL_TYPE, __string__,
      create_argon_native_function("__string__", ARGON_BOOL_TYPE___string__));
  add_builtin_field(
      ARGON_BOOL_TYPE, __number__,
      create_argon_native_function("__number__", ARGON_BOOL_TYPE___number__));
  ADDITION_FUNCTION =
      create_argon_native_function("add", ARGON_ADDITION_FUNCTION);
  SUBTRACTION_FUNCTION =
      create_argon_native_function("subtract", ARGON_SUBTRACTION_FUNCTION);
  MULTIPLY_FUNCTION =
      create_argon_native_function("multiply", ARGON_MULTIPLY_FUNCTION);
  EXPONENT_FUNCTION =
      create_argon_native_function("multiply", ARGON_EXPONENT_FUNCTION);
  DIVIDE_FUNCTION =
      create_argon_native_function("divide", ARGON_DIVIDE_FUNCTION);
  FLOOR_DIVIDE_FUNCTION =
      create_argon_native_function("floor_divide", ARGON_FLOOR_DIVIDE_FUNCTION);
  MODULO_FUNCTION =
      create_argon_native_function("modulo", ARGON_MODULO_FUNCTION);
  EQUAL_FUNCTION = create_argon_native_function("equal", ARGON_EQUAL_FUNCTION);
  NOT_EQUAL_FUNCTION =
      create_argon_native_function("not_equal", ARGON_NOT_EQUAL_FUNCTION);
  LESS_THAN_FUNCTION =
      create_argon_native_function("less_than", ARGON_LESS_THAN_FUNCTION);
  LESS_THAN_EQUAL_FUNCTION = create_argon_native_function(
      "less_than_equal", ARGON_LESS_THAN_EQUAL_FUNCTION);
  GREATER_THAN_EQUAL_FUNCTION = create_argon_native_function(
      "greater_than_equal", ARGON_GREATER_THAN_EQUAL_FUNCTION);
  add_builtin_field(BASE_CLASS, __getattribute__,
                    create_argon_native_function("__getattribute__",
                                                 BASE_CLASS___getattribute__));
  add_builtin_field(
      BASE_CLASS, __setattr__,
      create_argon_native_function("__setattr__", BASE_CLASS___setattr__));
  create_ARGON_DICTIONARY_TYPE();
  create_ARGON_NUMBER_TYPE();
  create_ARGON_BUFFER_TYPE();

  init_array_type();
  init_tuple_type();

  native_api.ARGON_NULL = ARGON_NULL;
  native_api.ARGON_TRUE = ARGON_TRUE;
  native_api.ARGON_FALSE = ARGON_FALSE;
}

void add_to_hashmap(struct hashmap_GC *hashmap, char *name,
                    ArgonObject *value) {
  size_t length = strlen(name);
  uint64_t hash = siphash64_bytes(name, length, siphash_key);
  ArgonObject *key = new_string_object(name, length, 0, hash);
  hashmap_insert_GC(hashmap, hash, key, value, 0);
}

void bootstrap_globals() {
  Global_Scope = create_scope(NULL, true);
  add_to_scope(Global_Scope, "global", create_dictionary(Global_Scope->scope));
  add_to_scope(Global_Scope, "object", BASE_CLASS);
  add_to_scope(Global_Scope, "string", ARGON_STRING_TYPE);
  add_to_scope(Global_Scope, "type", ARGON_TYPE_TYPE);
  add_to_scope(Global_Scope, "boolean", ARGON_BOOL_TYPE);
  add_to_scope(Global_Scope, "number", ARGON_NUMBER_TYPE);
  add_to_scope(Global_Scope, "dictionary", ARGON_DICTIONARY_TYPE);
  add_to_scope(Global_Scope, "buffer", ARGON_BUFFER_TYPE);
  add_to_scope(Global_Scope, "tuple", ARGON_TUPLE_TYPE);

  add_to_scope(Global_Scope, "add", ADDITION_FUNCTION);
  add_to_scope(Global_Scope, "subtract", SUBTRACTION_FUNCTION);
  add_to_scope(Global_Scope, "multiply", MULTIPLY_FUNCTION);
  add_to_scope(Global_Scope, "exponent", EXPONENT_FUNCTION);
  add_to_scope(Global_Scope, "divide", DIVIDE_FUNCTION);
  add_to_scope(Global_Scope, "floor_divide", FLOOR_DIVIDE_FUNCTION);
  add_to_scope(Global_Scope, "modulo", MODULO_FUNCTION);
  add_to_scope(Global_Scope, "equal", EQUAL_FUNCTION);
  add_to_scope(Global_Scope, "not_equal", NOT_EQUAL_FUNCTION);
  add_to_scope(Global_Scope, "less_than", LESS_THAN_FUNCTION);
  add_to_scope(Global_Scope, "less_than_equal", LESS_THAN_EQUAL_FUNCTION);
  add_to_scope(Global_Scope, "greater_than", GREATER_THAN_FUNCTION);
  add_to_scope(Global_Scope, "greater_than_equal", GREATER_THAN_EQUAL_FUNCTION);

  // create platform
  hashmap_GC *platform = createHashmap_GC();
  add_to_hashmap(platform, "os",
                 new_string_object_null_terminated(PLATFORM_OS));
  add_to_hashmap(platform, "lib_prefix",
                 new_string_object_null_terminated(PLATFORM_LIB_PREFIX));
  add_to_hashmap(platform, "lib_ext",
                 new_string_object_null_terminated(PLATFORM_LIB_EXT));
  add_to_scope(Global_Scope, "platform", create_dictionary(platform));

  struct hashmap_GC *argon_term = createHashmap_GC();
  add_to_hashmap(argon_term, "log",
                 create_argon_native_function("log", term_log));
  add_to_scope(Global_Scope, "term", create_dictionary(argon_term));
  add_to_scope(
      Global_Scope, "load_native_code",
      create_argon_native_function("load_native_code", ARGON_LOAD_NATIVE_CODE));

  struct hashmap_GC *environment_variables = createHashmap_GC();
#if defined(_WIN32)
  // Windows: use WinAPI
  LPCH env = GetEnvironmentStringsA();
  if (!env)
    return;

  for (LPCH var = env; *var; var += strlen(var) + 1) {
    // Each string is like "KEY=VALUE"
    const char *equals = strchr(var, '=');
    if (equals) {
      size_t key_len = equals - var;
      char key[256];
      if (key_len >= sizeof(key))
        key_len = sizeof(key) - 1;
      strncpy(key, var, key_len);
      key[key_len] = '\0';

      const char *value = getenv(key);
      add_to_hashmap(environment_variables, key,
                     value ? new_string_object_null_terminated((char *)value)
                           : ARGON_NULL);
    }
  }

  FreeEnvironmentStringsA(env);
#else
  // POSIX systems: use environ
  for (char **env = environ; *env != NULL; env++) {
    const char *equals = strchr(*env, '=');
    if (equals) {
      size_t key_len = equals - *env;
      char key[256];
      if (key_len >= sizeof(key))
        key_len = sizeof(key) - 1;
      strncpy(key, *env, key_len);
      key[key_len] = '\0';

      const char *value = getenv(key);
      add_to_hashmap(environment_variables, key,
                     value ? new_string_object_null_terminated((char *)value)
                           : ARGON_NULL);
    }
  }
#endif

  add_to_scope(Global_Scope, "env", create_dictionary(environment_variables));
}

int compare_by_order(const void *a, const void *b) {
  const struct node_GC *itemA = (const struct node_GC *)a;
  const struct node_GC *itemB = (const struct node_GC *)b;
  return itemA->order - itemB->order;
}

static inline void load_const(uint8_t to_register, size_t length,
                              uint64_t offset, Translated *translated,
                              RuntimeState *state) {
  ArgonObject *object = new_string_object(
      arena_get(&translated->constants, offset), length, 0, 0);
  state->registers[to_register] = object;
}

struct hashmap_GC *runtime_hash_table = NULL;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash) {
  if (prehash) {
    void *result = hashmap_lookup_GC(runtime_hash_table, prehash);
    if (result) {
      return (uint64_t)result;
    }
  }
  uint64_t hash = siphash64_bytes(data, len, siphash_key);
  if (prehash)
    hashmap_insert_GC(runtime_hash_table, prehash, 0, (void *)hash, 0);
  return hash;
}

static inline void load_variable(int64_t length, int64_t offset,
                                 uint64_t prehash, Translated *translated,
                                 RuntimeState *state, struct Stack *stack,
                                 ArErr *err) {
  uint64_t hash =
      runtime_hash(arena_get(&translated->constants, offset), length, prehash);
  struct Stack *current_stack = stack;
  while (current_stack) {
    ArgonObject *result = hashmap_lookup_GC(current_stack->scope, hash);
    if (result) {
      state->registers[0] = result;
      return;
    }
    current_stack = current_stack->prev;
  }
  *err = create_err(state->source_location.line, state->source_location.column,
                    state->source_location.length, state->path, "Name Error",
                    "name '%.*s' is not defined", (int)length,
                    arena_get(&translated->constants, offset));
  return;
}

void init_runtime_state(RuntimeState *runtime, Translated translated,
                        char *path) {
  *runtime =
      (RuntimeState){ar_alloc(translated.registerCount * sizeof(ArgonObject *)),
                     0,
                     NULL,
                     NULL,
                     {0, 0, 0},
                     {},
                     createHashmap_GC(),
                     path,
                     0};
}

Stack *create_scope(Stack *prev, bool force) {
  if (force || !prev || prev->scope->count) {
    Stack *stack = ar_alloc(sizeof(Stack));
    stack->fake_new_scopes = 0;
    stack->scope = createHashmap_GC();
    stack->prev = prev;
    return stack;
  }
  prev->fake_new_scopes++;
  return prev;
}

void add_to_scope(Stack *stack, char *name, ArgonObject *value) {
  size_t length = strlen(name);
  uint64_t hash = siphash64_bytes(name, length, siphash_key);
  ArgonObject *key = new_string_object(name, length, 0, hash);
  hashmap_insert_GC(stack->scope, hash, key, value, 0);
}

void add_source_location_to_error_if_not_exists(ArErr *err,
                                                RuntimeState *state) {
  if (err->exists && !strlen(err->path)) {
    err->column = state->source_location.column;
    err->length = state->source_location.length;
    err->line = state->source_location.line;
    strcpy(err->path, state->path);
  }
}

void runtime(Translated _translated, RuntimeState _state, Stack *stack,
             ArErr *err_ptr) {
  static void *const dispatch_table[] = {
      [OP_LOAD_STRING] = &&DO_LOAD_STRING,
      [OP_DECLARE] = &&DO_DECLARE,
      [OP_LOAD_NULL] = &&DO_LOAD_NULL,
      [OP_LOAD_FUNCTION] = &&DO_LOAD_FUNCTION,
      [OP_IDENTIFIER] = &&DO_IDENTIFIER,
      [OP_BOOL] = &&DO_BOOL,
      [OP_JUMP_IF_FALSE] = &&DO_JUMP_IF_FALSE,
      [OP_JUMP] = &&DO_JUMP,
      [OP_NEW_SCOPE] = &&DO_NEW_SCOPE,
      [OP_EMPTY_SCOPE] = &&DO_EMPTY_SCOPE,
      [OP_POP_SCOPE] = &&DO_POP_SCOPE,
      [OP_INIT_CALL] = &&DO_INIT_CALL,
      [OP_INSERT_ARG] = &&DO_INSERT_ARG,
      [OP_CALL] = &&DO_CALL,
      [OP_SOURCE_LOCATION] = &&DO_SOURCE_LOCATION,
      [OP_LOAD_BOOL] = &&DO_LOAD_BOOL,
      [OP_LOAD_NUMBER] = &&DO_LOAD_NUMBER,
      [OP_ASSIGN] = &&DO_ASSIGN,
      [OP_COPY_TO_REGISTER] = &&DO_COPY_TO_REGISTER,
      [OP_ADDITION] = &&DO_ADDITION,
      [OP_SUBTRACTION] = &&DO_SUBTRACTION,
      [OP_LOAD_GETATTRIBUTE_METHOD] = &&DO_LOAD_GETATTRIBUTE_METHOD,
      [OP_MULTIPLICATION] = &&DO_MULTIPLICATION,
      [OP_EXPONENTIATION] = &&DO_EXPONENTIATION,
      [OP_DIVISION] = &&DO_DIVISION,
      [OP_FLOOR_DIVISION] = &&DO_FLOOR_DIVISION,
      [OP_EQUAL] = &&DO_EQUAL,
      [OP_NOT_EQUAL] = &&DO_NOT_EQUAL,
      [OP_LESS_THAN] = &&DO_LESS_THAN,
      [OP_LESS_THAN_EQUAL] = &&DO_LESS_THAN_EQUAL,
      [OP_GREATER_THAN] = &&DO_GREATER_THAN,
      [OP_GREATER_THAN_EQUAL] = &&DO_GREATER_THAN_EQUAL,
      [OP_MODULO] = &&DO_MODULO,
      [OP_NOT] = &&DO_NOT,
      [OP_NEGATION] = &&DO_NEGATION,
      [OP_LOAD_SETATTR_METHOD] = &&DO_LOAD_SETATTR_METHOD,
      [OP_CREATE_DICTIONARY] = &&DO_CREATE_DICTIONARY,
      [OP_LOAD_SETITEM_METHOD] = &&DO_LOAD_SETITEM_METHOD,
      [OP_LOAD_GETITEM_METHOD] = &&DO_LOAD_GETITEM_METHOD,
      [OP_LOAD_BASE_CLASS] = &&DO_LOAD_BASE_CLASS,
      [OP_CREATE_CLASS] = &&DO_CREATE_CLASS,
      [OP_IMPORT] = &&DO_IMPORT,
      [OP_EXPOSE_ALL] = &&DO_EXPOSE_ALL,
      [OP_EXPOSE] = &&DO_EXPOSE,
      [OP_LOAD_CREATE_ARRAY] = &&DO_LOAD_CREATE_ARRAY,
      [OP_NOT_IN] = &&DO_NOT_IN,
      [OP_IN] = &&DO_IN};
  _state.head = 0;

  ArErr err = *err_ptr;

  StackFrame *currentStackFrame = GC_MALLOC_UNCOLLECTABLE(sizeof(StackFrame));
  *currentStackFrame = (StackFrame){_translated, _state, stack, NULL, 0, {}};
  currentStackFrame->state.currentStackFramePointer = &currentStackFrame;
  while (currentStackFrame) {
    size_t ip = currentStackFrame->state.head;
    DArray *bytecode = &currentStackFrame->translated.bytecode;
    size_t bytecode_size = bytecode->size;
    uint8_t *bc = bytecode->data;
    Translated *translated = &currentStackFrame->translated;
    RuntimeState *state = &currentStackFrame->state;
    while (ip < bytecode_size && !err.exists) {

      uint8_t instruction = POP_BYTE();
      // printf("instruction: %d\n", instruction);
      // for (size_t i = 0; i < translated->bytecode.size; i++) {
      //   if (i == state->head)
      //     printf("\n");
      //   printf("%d ", ((uint8_t *)translated->bytecode.data)[i]);
      // }
      // printf("\n");
      goto *dispatch_table[instruction];
    DO_LOAD_NULL:
      state->registers[POP_BYTE()] = ARGON_NULL;
      continue;
    DO_LOAD_BASE_CLASS:
      state->registers[0] = BASE_CLASS;
      continue;
    DO_LOAD_STRING: {
      uint8_t to_register = POP_BYTE();
      size_t length;
      POP_U64(length);
      uint64_t offset;
      POP_U64(offset);
      load_const(to_register, length, offset, translated, state);
      continue;
    }
    DO_LOAD_NUMBER: {

      uint8_t to_register = POP_BYTE();
      uint8_t is_int64 = POP_BYTE();
      ArgonObject *cache_number = NULL;
      if (is_int64) {
        int64_t num;
        POP_U64(num);
        // bool small_num = num < small_ints_min || num > small_ints_max;
        // if (small_num) {
        //   cache_number = hashmap_lookup_GC(state->load_number_cache, num);
        // }
        // if (cache_number) {
        //   state->registers[to_register] = cache_number;
        //   continue;
        // }
        state->registers[to_register] = new_number_object_from_int64(num);
        // if (small_num) {
        //   hashmap_insert_GC(state->load_number_cache, num, NULL,
        //                     state->registers[to_register], 0);
        // }
        continue;
      }
      size_t num_size;
      POP_U64(num_size);
      size_t num_pos;
      POP_U64(num_pos);
      bool is_int = POP_BYTE();
      bool is_negative = POP_BYTE();
      size_t den_size = 0;
      size_t den_pos = 0;
      if (!is_int) {
        POP_U64(den_size);
        POP_U64(den_pos);
      }

      uint64_t uuid =
          make_id(num_size, num_pos, is_int, is_negative, den_size, den_pos);

      cache_number = hashmap_lookup_GC(state->load_number_cache, uuid);
      if (cache_number) {
        state->registers[to_register] = cache_number;
        state->head += 16;
        if (!is_int) {
          state->head += 16;
        }
        continue;
      }

      mpq_t r;
      mpq_init(r);
      mpz_t num;
      mpz_init(num);
      mpz_import(num, num_size, 1, 1, 0, 0,
                 arena_get(&translated->constants, num_pos));
      mpq_set_num(r, num);
      if (is_negative) {
        mpq_neg(r, r);
      }
      mpz_clear(num);
      if (!is_int) {
        mpz_t den;
        mpz_init(den);
        mpz_import(den, den_size, 1, 1, 0, 0,
                   arena_get(&translated->constants, den_pos));
        mpq_set_den(r, den);
        mpz_clear(den);
      } else {
        mpz_set_si(mpq_denref(r), 1);
      }

      state->registers[to_register] = new_number_object(r);
      hashmap_insert_GC(state->load_number_cache, uuid, NULL,
                        state->registers[to_register], 0);
      mpq_clear(r);
      continue;
    }
    DO_LOAD_FUNCTION: {
      uint64_t offset;
      POP_U64(offset);
      uint64_t length;
      POP_U64(length);
      uint64_t number_of_parameters;
      POP_U64(number_of_parameters);
      ArgonObject *object =
          new_instance(ARGON_FUNCTION_TYPE,
                       sizeof(struct argon_function_struct) +
                           number_of_parameters * sizeof(struct string_struct));
      object->type = TYPE_FUNCTION;
      add_builtin_field(
          object, __name__,
          new_string_object(arena_get(&translated->constants, offset), length,
                            0, 0));
      object->value.argon_fn =
          (struct argon_function_struct *)((char *)object +
                                           sizeof(ArgonObject));
      object->value.argon_fn->parameters =
          (struct string_struct *)((char *)object->value.argon_fn +
                                   sizeof(struct argon_function_struct));
      object->value.argon_fn->translated = *translated;
      object->value.argon_fn->number_of_parameters = number_of_parameters;
      for (size_t i = 0; i < object->value.argon_fn->number_of_parameters;
           i++) {
        POP_U64(offset);
        POP_U64(length);
        object->value.argon_fn->parameters[i].data =
            arena_get(&translated->constants, offset);
        object->value.argon_fn->parameters[i].length = length;
      }
      POP_U64(offset);
      POP_U64(length);
      object->value.argon_fn->bytecode =
          arena_get(&translated->constants, offset);
      object->value.argon_fn->bytecode_length = length;
      object->value.argon_fn->stack = currentStackFrame->stack;
      state->registers[0] = object;
      continue;
    }
    DO_IMPORT:
      runtime_import(state, &err);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    DO_EXPOSE_ALL: {
      size_t nodes_length;
      hashmap_GC *hashmap = state->registers[0]->value.as_hashmap;
      struct node_GC **nodes = hashmap_GC_to_array(hashmap, &nodes_length);
      hashmap_GC *scope_hashmap = stack->scope;

      for (size_t i = 0; i < nodes_length; i++) {
        hashmap_insert_GC(scope_hashmap, nodes[i]->hash, nodes[i]->key,
                          nodes[i]->val, 0);
      }
      continue;
    }
    DO_EXPOSE: {
      hashmap_GC *hashmap = state->registers[0]->value.as_hashmap;

      int64_t length;
      POP_U64(length);
      int64_t offset;
      POP_U64(offset);
      uint64_t prehash;
      POP_U64(prehash);
      uint64_t hash = runtime_hash(arena_get(&translated->constants, offset),
                                   length, prehash);

      ArgonObject *object = hashmap_lookup_GC(hashmap, hash);
      if (!object) {
        err = create_err(state->source_location.line,
                         state->source_location.column,
                         state->source_location.length, state->path,
                         "Runtime Error", "could not find '%.*s'", length,
                         arena_get(&translated->constants, offset));
        continue;
      }
      state->registers[0] = object;
      continue;
    }
    DO_IDENTIFIER: {
      int64_t length;
      POP_U64(length);
      int64_t offset;
      POP_U64(offset);
      uint64_t prehash;
      POP_U64(prehash);
      load_variable(length, offset, prehash, translated, state,
                    currentStackFrame->stack, &err);
      continue;
    }
    DO_DECLARE: {
      int64_t length;
      POP_U64(length);
      int64_t offset;
      POP_U64(offset);
      uint64_t prehash;
      POP_U64(prehash);
      uint8_t from_register = POP_BYTE();
      runtime_declaration(length, offset, prehash, from_register, translated,
                          state, currentStackFrame->stack, &err);
      continue;
    }
    DO_ASSIGN: {
      int64_t length;
      POP_U64(length);
      int64_t offset;
      POP_U64(offset);
      uint64_t prehash;
      POP_U64(prehash);
      uint8_t from_register = POP_BYTE();
      runtime_assignment(length, offset, prehash, from_register, translated,
                         state, currentStackFrame->stack);
      continue;
    }
    DO_BOOL: {
      if (state->registers[0] == ARGON_TRUE ||
          state->registers[0] == ARGON_FALSE)
        continue;
      if ((state->registers[0]->type != TYPE_OBJECT)) {
        state->registers[0] =
            state->registers[0]->as_bool ? ARGON_TRUE : ARGON_FALSE;
        continue;
      }
      ArgonObject *args[] = {ARGON_BOOL_TYPE, state->registers[0]};
      state->registers[0] = ARGON_BOOL_TYPE___new__(2, args, &err, state, NULL);
      continue;
    }
    DO_JUMP_IF_FALSE: {
      uint8_t from_register = POP_BYTE();
      uint64_t to;
      POP_U64(to);
      if (state->registers[from_register] == ARGON_FALSE) {
        ip = to;
      }
      continue;
    }
    DO_NOT:
      state->registers[0] =
          state->registers[0] == ARGON_FALSE ? ARGON_TRUE : ARGON_FALSE;
      continue;
    DO_JUMP: {
      uint64_t to;
      POP_U64(to);
      ip = to;
      continue;
    }
    DO_NEW_SCOPE:
      currentStackFrame->stack = create_scope(currentStackFrame->stack, false);
      continue;
    DO_EMPTY_SCOPE:
      clear_hashmap_GC(currentStackFrame->stack->scope);
      continue;
    DO_POP_SCOPE:
      if (currentStackFrame->stack->fake_new_scopes) {
        currentStackFrame->stack->fake_new_scopes--;
        goto DO_EMPTY_SCOPE;
      }
      currentStackFrame->stack = currentStackFrame->stack->prev;
      continue;
    DO_INIT_CALL: {
      size_t length;
      POP_U64(length);
      call_instance call_instance = {state->call_instance, state->registers[0],
                                     ar_alloc(length * sizeof(ArgonObject *)),
                                     length};
      state->call_instance = ar_alloc(sizeof(call_instance));
      *state->call_instance = call_instance;
      continue;
    }
    DO_INSERT_ARG:;
      size_t index;
      POP_U64(index);
      state->call_instance->args[index] = state->registers[0];
      continue;
    DO_CALL: {
      state->head = ip;
      run_call(state->call_instance->to_call, state->call_instance->args_length,
               state->call_instance->args, state, false, &err);
      state->call_instance = (*state->call_instance).previous;
      ip = currentStackFrame->state.head;
      bytecode = &currentStackFrame->translated.bytecode;
      bytecode_size = bytecode->size;
      bc = bytecode->data;
      translated = &currentStackFrame->translated;
      state = &currentStackFrame->state;
      continue;
    }
    DO_SOURCE_LOCATION: {
      uint64_t line;
      POP_U64(line);
      uint64_t column;
      POP_U64(column);
      uint64_t length;
      POP_U64(length);
      state->source_location = (SourceLocation){line, column, length};
      currentStackFrame->source_location = state->source_location;
      continue;
    }
    DO_LOAD_BOOL:
      state->registers[0] = POP_BYTE() ? ARGON_TRUE : ARGON_FALSE;
      continue;
    DO_NEGATION:
      if (state->registers[0]->type == TYPE_NUMBER) {
        ArgonObject *value = state->registers[0];
        if ((value->value.as_number->is_int64)) {
          int64_t a = value->value.as_number->n.i64;
          state->registers[0] = new_number_object_from_int64(-a);
          continue;
        }
        mpq_t result;
        mpq_init(result);
        mpq_neg(result, *value->value.as_number->n.mpq);
        state->registers[0] = new_number_object(result);
        mpq_clear(result);
        continue;
      }
      ArgonObject *negation_function = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __negation__,
          state->registers[0]);
      if (!state->registers[0]) {
        err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __negation__ from objects class");
        continue;
      }
      ArgonObject *args[] = {};
      argon_call(negation_function, 0, args, &err, state);
      continue;
    DO_LOAD_GETATTRIBUTE_METHOD:
      state->registers[0] = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __getattribute__,
          state->registers[0]);
      if (!state->registers[0]) {
        err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __getattribute__ from objects class");
      }
      continue;
    DO_COPY_TO_REGISTER: {
      uint8_t from_register = POP_BYTE();
      uint64_t to_register = POP_BYTE();
      state->registers[to_register] = state->registers[from_register];
      continue;
    }
    DO_ADDITION: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          bool gonna_overflow = (a > 0 && b > 0 && a > INT64_MAX - b) ||
                                (a < 0 && b < 0 && a < INT64_MIN - b);
          if (!gonna_overflow) {
            state->registers[registerC] = new_number_object_from_int64(a + b);
            continue;
          }
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          mpq_set_si64(a_GMP, a, 1);
          mpq_set_si64(b_GMP, b, 1);
          mpq_add(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          mpq_t r;
          mpq_init(r);
          mpq_add(r, *valueA->value.as_number->n.mpq,
                  *valueB->value.as_number->n.mpq);
          state->registers[registerC] = new_number_object(r);
          mpq_clear(r);
        } else {
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          if (valueA->value.as_number->is_int64) {
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_add(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(ADDITION_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }
    DO_SUBTRACTION: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          int64_t neg_a = -a;
          bool gonna_overflow = (neg_a > 0 && b > 0 && b > INT64_MAX - neg_a) ||
                                (neg_a < 0 && b < 0 && b < INT64_MIN - neg_a);
          if (!gonna_overflow) {
            state->registers[registerC] = new_number_object_from_int64(a - b);
            continue;
          }
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          mpq_set_si64(a_GMP, a, 1);
          mpq_set_si64(b_GMP, b, 1);
          mpq_sub(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          mpq_t r;
          mpq_init(r);
          mpq_sub(r, *valueA->value.as_number->n.mpq,
                  *valueB->value.as_number->n.mpq);
          state->registers[registerC] = new_number_object(r);
          mpq_clear(r);
        } else {
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          if (valueA->value.as_number->is_int64) {
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_sub(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(SUBTRACTION_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }
    DO_MULTIPLICATION: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          bool gonna_overflow =
              a > 0 ? (b > 0 ? a > INT64_MAX / b : b < INT64_MIN / a)
                    : (b > 0 ? a < INT64_MIN / b : a != 0 && b < INT64_MAX / a);
          if (!gonna_overflow) {
            state->registers[registerC] = new_number_object_from_int64(a * b);
            continue;
          }
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          mpq_set_si64(a_GMP, a, 1);
          mpq_set_si64(b_GMP, b, 1);
          mpq_mul(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          mpq_t r;
          mpq_init(r);
          mpq_mul(r, *valueA->value.as_number->n.mpq,
                  *valueB->value.as_number->n.mpq);
          state->registers[registerC] = new_number_object(r);
          mpq_clear(r);
        } else {
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          if (valueA->value.as_number->is_int64) {
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_mul(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(MULTIPLY_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }
    DO_EXPONENTIATION: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {

        /* ---------- fast int64 ^ int64 path ---------- */
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {

          int64_t base = valueA->value.as_number->n.i64;
          int64_t exp = valueB->value.as_number->n.i64;

          /* negative exponent → rational */
          if (exp < 0) {
            mpq_t a, b, r;
            mpq_init(a);
            mpq_init(b);
            mpq_init(r);

            mpq_set_si64(a, base, 1);
            mpq_set_si64(b, exp, 1);

            mpq_pow_q(r, a, b);

            state->registers[registerC] = new_number_object(r);

            mpq_clear(a);
            mpq_clear(b);
            mpq_clear(r);
            continue;
          }

          /* exp >= 0 → try int64 exponentiation */
          bool overflow = false;
          int64_t result = 1;
          int64_t a = base;
          int64_t e = exp;

          while (e > 0) {
            if (e & 1) {
              if (__builtin_mul_overflow(result, a, &result)) {
                overflow = true;
                break;
              }
            }
            e >>= 1;
            if (e && __builtin_mul_overflow(a, a, &a)) {
              overflow = true;
              break;
            }
          }

          if (!overflow) {
            state->registers[registerC] = new_number_object_from_int64(result);
            continue;
          }

          /* overflow → fall through to GMP */
        }

        /* ---------- GMP / rational path ---------- */
        {
          mpq_t a_GMP, b_GMP, r;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          mpq_init(r);

          /* load base */
          if (valueA->value.as_number->is_int64)
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
          else
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);

          /* load exponent */
          if (valueB->value.as_number->is_int64)
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          else
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);

          /* ---------- REAL-NUMBER DOMAIN CHECK ---------- */

          /* 0^0 or 0^negative */
          if (mpq_sgn(a_GMP) == 0 && mpq_sgn(b_GMP) <= 0) {
            err = create_err(
                state->source_location.line, state->source_location.column,
                state->source_location.length, state->path, "Math Error",
                "0 cannot be raised to zero or a negative power");

            mpq_clear(a_GMP);
            mpq_clear(b_GMP);
            mpq_clear(r);
            continue;
          }

          /* negative base with non-integer exponent → complex */
          if (mpq_sgn(a_GMP) < 0 && mpz_cmp_ui(mpq_denref(b_GMP), 1) != 0) {
            err = create_err(
                state->source_location.line, state->source_location.column,
                state->source_location.length, state->path, "Math Error",
                "Negative base with fractional exponent is not a real number");

            mpq_clear(a_GMP);
            mpq_clear(b_GMP);
            mpq_clear(r);
            continue;
          }

          /* ---------- SAFE TO COMPUTE ---------- */

          mpq_pow_q(r, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(r);

          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
          mpq_clear(r);
          continue;
        }
      }

      /* ---------- fallback to dynamic dispatch ---------- */
      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(EXPONENT_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_DIVISION: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          if (!b) {
            err = create_err(state->source_location.line,
                             state->source_location.column,
                             state->source_location.length, state->path,
                             "Zero Division Error", "division by zero");
            continue;
          }
          if (b < 0) {
            a = -a;
            b = -b;
          }
          state->registers[registerC] =
              new_number_object_from_num_and_den(a, b);
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          mpq_t r;
          mpq_init(r);
          mpq_div(r, *valueA->value.as_number->n.mpq,
                  *valueB->value.as_number->n.mpq);
          state->registers[registerC] = new_number_object(r);
          mpq_clear(r);
        } else {
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          if (valueA->value.as_number->is_int64) {
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            if (!valueB->value.as_number->n.i64) {
              err = create_err(state->source_location.line,
                               state->source_location.column,
                               state->source_location.length, state->path,
                               "Zero Division Error", "division by zero");
              continue;
            }
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_div(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(DIVIDE_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_FLOOR_DIVISION: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          if (!b) {
            err = create_err(state->source_location.line,
                             state->source_location.column,
                             state->source_location.length, state->path,
                             "Zero Division Error", "floor division by zero");
            continue;
          }
          state->registers[registerC] = new_number_object_from_int64(a / b);
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          mpq_t r;
          mpq_init(r);
          mpq_fdiv(r, *valueA->value.as_number->n.mpq,
                   *valueB->value.as_number->n.mpq);
          state->registers[registerC] = new_number_object(r);
          mpq_clear(r);
        } else {
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          if (valueA->value.as_number->is_int64) {
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            if (!valueB->value.as_number->n.i64) {
              err = create_err(state->source_location.line,
                               state->source_location.column,
                               state->source_location.length, state->path,
                               "Zero Division Error", "floor division by zero");
              continue;
            }
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_fdiv(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(FLOOR_DIVIDE_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_MODULO: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          if (!b) {
            err = create_err(state->source_location.line,
                             state->source_location.column,
                             state->source_location.length, state->path,
                             "Zero Division Error", "modulo by zero");
            continue;
          }
          state->registers[registerC] = new_number_object_from_int64(a % b);
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          mpq_t r;
          mpq_init(r);
          mpq_fmod(r, *valueA->value.as_number->n.mpq,
                   *valueB->value.as_number->n.mpq);
          state->registers[registerC] = new_number_object(r);
          mpq_clear(r);
        } else {
          mpq_t a_GMP, b_GMP;
          mpq_init(a_GMP);
          mpq_init(b_GMP);
          if (valueA->value.as_number->is_int64) {
            mpq_set_si64(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            if (!valueB->value.as_number->n.i64) {
              err = create_err(state->source_location.line,
                               state->source_location.column,
                               state->source_location.length, state->path,
                               "Zero Division Error", "modulo by zero");
              continue;
            }
            mpq_set_si64(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_fmod(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(MODULO_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_EQUAL: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          state->registers[registerC] = a == b ? ARGON_TRUE : ARGON_FALSE;
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp(*valueA->value.as_number->n.mpq,
                      *valueB->value.as_number->n.mpq) == 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else if (valueA->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp_ui(*valueB->value.as_number->n.mpq,
                         valueA->value.as_number->n.i64, 1) == 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else {
          state->registers[registerC] =
              mpq_cmp_ui(*valueA->value.as_number->n.mpq,
                         valueB->value.as_number->n.i64, 1) == 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(EQUAL_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_NOT_EQUAL: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          state->registers[registerC] = a != b ? ARGON_TRUE : ARGON_FALSE;
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp(*valueA->value.as_number->n.mpq,
                      *valueB->value.as_number->n.mpq) != 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else if (valueA->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp_ui(*valueB->value.as_number->n.mpq,
                         valueA->value.as_number->n.i64, 1) != 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else {
          state->registers[registerC] =
              mpq_cmp_ui(*valueA->value.as_number->n.mpq,
                         valueB->value.as_number->n.i64, 1) != 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(NOT_EQUAL_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_LESS_THAN: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          state->registers[registerC] = a < b ? ARGON_TRUE : ARGON_FALSE;
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp(*valueA->value.as_number->n.mpq,
                      *valueB->value.as_number->n.mpq) < 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else if (valueA->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp_ui(*valueB->value.as_number->n.mpq,
                         valueA->value.as_number->n.i64, 1) > 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else {
          state->registers[registerC] =
              mpq_cmp_ui(*valueA->value.as_number->n.mpq,
                         valueB->value.as_number->n.i64, 1) < 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(LESS_THAN_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_GREATER_THAN: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          state->registers[registerC] = a > b ? ARGON_TRUE : ARGON_FALSE;
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp(*valueA->value.as_number->n.mpq,
                      *valueB->value.as_number->n.mpq) > 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else if (valueA->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp_ui(*valueB->value.as_number->n.mpq,
                         valueA->value.as_number->n.i64, 1) < 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else {
          state->registers[registerC] =
              mpq_cmp_ui(*valueA->value.as_number->n.mpq,
                         valueB->value.as_number->n.i64, 1) > 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(GREATER_THAN_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_LESS_THAN_EQUAL: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          state->registers[registerC] = a <= b ? ARGON_TRUE : ARGON_FALSE;
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp(*valueA->value.as_number->n.mpq,
                      *valueB->value.as_number->n.mpq) <= 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else if (valueA->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp_ui(*valueB->value.as_number->n.mpq,
                         valueA->value.as_number->n.i64, 1) >= 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else {
          state->registers[registerC] =
              mpq_cmp_ui(*valueA->value.as_number->n.mpq,
                         valueB->value.as_number->n.i64, 1) <= 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(LESS_THAN_EQUAL_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_GREATER_THAN_EQUAL: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER) {
        if ((valueA->value.as_number->is_int64 &&
             valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          state->registers[registerC] = a >= b ? ARGON_TRUE : ARGON_FALSE;
        } else if (!valueA->value.as_number->is_int64 &&
                   !valueB->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp(*valueA->value.as_number->n.mpq,
                      *valueB->value.as_number->n.mpq) >= 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else if (valueA->value.as_number->is_int64) {
          state->registers[registerC] =
              mpq_cmp_ui(*valueB->value.as_number->n.mpq,
                         valueA->value.as_number->n.i64, 1) <= 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        } else {
          state->registers[registerC] =
              mpq_cmp_ui(*valueA->value.as_number->n.mpq,
                         valueB->value.as_number->n.i64, 1) >= 0
                  ? ARGON_TRUE
                  : ARGON_FALSE;
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] =
          argon_call(GREATER_THAN_EQUAL_FUNCTION, 2, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      continue;
    }

    DO_NOT_IN:
    DO_IN: {
      uint8_t registerA = POP_BYTE();
      uint8_t registerB = POP_BYTE();
      uint8_t registerC = POP_BYTE();

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      ArgonObject *object_class = get_builtin_field(valueB, __class__);
      ArgonObject *object__contains__ =
          get_builtin_field_for_class(object_class, __contains__, valueB);
      if (!object__contains__) {
        ArgonObject *cls___name__ = get_builtin_field(object_class, __name__);
        err = create_err(0, 0, 0, "", "Runtime Error",
                         "Object of type '%.*s' is missing __contains__ method",
                         (int)cls___name__->value.as_str->length,
                         cls___name__->value.as_str->data);
        continue;
      }

      ArgonObject *args[] = {valueA};
      state->registers[registerC] =
          argon_call(object__contains__, 1, args, &err, state);
      add_source_location_to_error_if_not_exists(&err, state);
      if (instruction == OP_IN)
        continue;
      state->registers[registerC] =
          state->registers[registerC] == ARGON_FALSE ? ARGON_TRUE : ARGON_FALSE;
      continue;
    }

    DO_LOAD_SETATTR_METHOD: {
      state->registers[0] = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __setattr__,
          state->registers[0]);
      if (!state->registers[0]) {
        err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __setattr__ from objects class");
      }
      continue;
    }
    DO_CREATE_CLASS: {
      int64_t length;
      POP_U64(length);
      int64_t offset;
      POP_U64(offset);
      ArgonObject *class = new_class();
      add_builtin_field(class, __base__, state->registers[0]);
      add_builtin_field(
          class, __name__,
          new_string_object(arena_get(&translated->constants, offset), length,
                            0, 0));
      state->registers[0] = class;
      continue;
    }
    DO_CREATE_DICTIONARY: {
      state->registers[0] = create_dictionary(createHashmap_GC());
      continue;
    }
    DO_LOAD_SETITEM_METHOD: {
      state->registers[0] = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __setitem__,
          state->registers[0]);
      if (!state->registers[0]) {
        err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __setitem__ from objects class");
      }
      continue;
    }
    DO_LOAD_GETITEM_METHOD: {
      state->registers[0] = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __getitem__,
          state->registers[0]);
      if (!state->registers[0]) {
        err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __getitem__ from objects class");
      }
      continue;
    }
    DO_LOAD_CREATE_ARRAY: {
      state->registers[0] = ARGON_ARRAY_CREATE;
      continue;
    }
    }

    if (err.exists && currentStackFrame->source_location.length) {
      struct StackTraceFrame frame = {currentStackFrame->source_location.line,
                                      currentStackFrame->source_location.column,
                                      currentStackFrame->translated.path};
      darray_armem_insert(err.stack_trace, err.stack_trace->size, &frame);
    }

    ArgonObject *result = currentStackFrame->state.registers[0];
    currentStackFrame = currentStackFrame->previousStackFrame;
    if (currentStackFrame)
      currentStackFrame->state.registers[0] = result;
  }
  if (err.exists)
    *err_ptr = err;
}
