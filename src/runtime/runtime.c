/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "runtime.h"
#include "../err.h"
#include "../hash_data/hash_data.h"
#include "../parser/number/number.h"
#include "../translator/translator.h"
#include "assignment/assignment.h"
#include "call/call.h"
#include "declaration/declaration.h"
#include "internals/hashmap/hashmap.h"
#include "objects/dictionary/dictionary.h"
#include "objects/functions/functions.h"
#include "objects/literals/literals.h"
#include "objects/number/number.h"
#include "objects/object.h"
#include "objects/string/string.h"
#include "objects/term/term.h"
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

ArgonObject *ARGON_METHOD_TYPE;
Stack *Global_Scope = NULL;
ArgonObject *ADDITION_FUNCTION;
ArgonObject *SUBTRACTION_FUNCTION;
ArgonObject *MULTIPLY_FUNCTION;
ArgonObject *DIVIDE_FUNCTION;
ArgonObject *POWER_FUNCTION;
ArgonObject *MODULO_FUNCTION;
ArgonObject *FLOORDIVIDE_FUNCTION;

ArgonObject *BASE_CLASS___getattribute__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state) {
  (void)state;
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
  ArgonObject *value = get_field_l(to_access, access->value.as_str->data, hash,
                                   access->value.as_str->length, true, false);
  if (value)
    return value;
  ArgonObject *cls__get_attr__ = get_builtin_field_for_class(
      get_builtin_field(to_access, __class__), __get_attr__, to_access);
  if (cls__get_attr__) {
    value =
        argon_call(cls__get_attr__, 1, (ArgonObject *[]){access}, err, state);
    if (err->exists) {
      return ARGON_NULL;
    }
    return value;
  }
  ArgonObject *name = get_builtin_field_for_class(
      get_builtin_field(to_access, __class__), __name__, to_access);
  *err = create_err(
      0, 0, 0, "", "Runtime Error", "'%.*s' object has no attribute '%.*s'",
      (int)name->value.as_str->length, name->value.as_str->data,
      (int)access->value.as_str->length, access->value.as_str->data);
  return ARGON_NULL;
}

ArgonObject *ARGON_ADDITION_FUNCTION(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "add expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *object__add__ = get_builtin_field_for_class(
        get_builtin_field(output, __class__), __add__, output);
    if (!object__add__) {
      ArgonObject *cls___name__ = get_builtin_field(output, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __add__ method",
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
                                        ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "subtract expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *function__subtract__ = get_builtin_field_for_class(
        get_builtin_field(output, __class__), __subtract__, output);
    if (!function__subtract__) {
      ArgonObject *cls___name__ = get_builtin_field(output, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __subtract__ method",
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
                                     ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "multiply expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *function__multiply__ = get_builtin_field_for_class(
        get_builtin_field(output, __class__), __multiply__, output);
    if (!function__multiply__) {
      ArgonObject *cls___name__ = get_builtin_field(output, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __multiply__ method",
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
                                   RuntimeState *state) {
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "divide expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *function___divide__ = get_builtin_field_for_class(
        get_builtin_field(output, __class__), __divide__, output);
    if (!function___divide__) {
      ArgonObject *cls___name__ = get_builtin_field(output, __name__);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __divide__ method",
                        (int)cls___name__->value.as_str->length,
                        cls___name__->value.as_str->data);
      return ARGON_NULL;
    }
    output = argon_call(function___divide__, 1, (ArgonObject *[]){argv[i]}, err,
                        state);
  }
  return output;
}

ArgonObject *ARGON_TYPE_TYPE___call__(size_t argc, ArgonObject **argv,
                                      ArErr *err, RuntimeState *state) {
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
      ARGON_TYPE_TYPE___call___args, err, state);
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
                                RuntimeState *state) {
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
                                RuntimeState *state) {
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj = new_instance(argv[0]);
  return new_obj;
}

ArgonObject *BASE_CLASS___init__(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state) {
  (void)state;
  (void)argv;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__init__ expects 1 argument, got %" PRIu64, argc);
  }
  return ARGON_NULL;
}

ArgonObject *BASE_CLASS___setattr__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state) {
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__setattr__ expects 3 argument, got %" PRIu64, argc);
  }
  if (!argv[1]->value.as_str->hash)
    argv[1]->value.as_str->hash =
        runtime_hash(argv[1]->value.as_str->data, argv[1]->value.as_str->length,
                     argv[1]->value.as_str->prehash);
  add_field_l(argv[0], argv[1]->value.as_str->data, argv[1]->value.as_str->hash,
              argv[1]->value.as_str->length, argv[2]);
  return argv[2];
}

ArgonObject *BASE_CLASS___string__(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
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
  else
    snprintf(buffer, sizeof(buffer), "<object at %p>", argv[0]);
  return new_string_object_null_terminated(buffer);
}

ArgonObject *BASE_CLASS___repr__(size_t argc, ArgonObject **argv, ArErr *err,
                                 RuntimeState *state) {
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__repr__ expects 1 arguments, got %" PRIu64, argc);
  }
  ArgonObject *string_method = get_builtin_field_for_class(
      get_builtin_field(argv[0], __class__), __string__, argv[0]);
  return argon_call(string_method, 0, NULL, err, state);
}

ArgonObject *BASE_CLASS___boolean__(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state) {
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
  }
  return ARGON_TRUE;
}

ArgonObject *ARGON_STRING_TYPE___init__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
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
                                     ArErr *err, RuntimeState *state) {
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
                                       ArErr *err, RuntimeState *state) {
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
                                        ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_string_object_null_terminated(argv[0] == ARGON_TRUE ? "true"
                                                                 : "false");
}

ArgonObject *ARGON_BOOL_TYPE___number__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__number__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return new_number_object_from_int64(argv[0] == ARGON_TRUE);
}

ArgonObject *ARGON_STRING_TYPE___string__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
  }
  return argv[0];
}
ArgonObject *ARGON_STRING_TYPE___repr__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__repr__ expects 1 arguments, got %" PRIu64, argc);
  }
  char *quoted = c_quote_string(argv[0]->value.as_str->data,
                                argv[0]->value.as_str->length);
  ArgonObject *result = new_string_object_null_terminated(quoted);
  free(quoted);
  return result;
}
ArgonObject *ARGON_STRING_TYPE___hash__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__hash__ expects 1 arguments, got %" PRIu64, argc);
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
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__number__ expects 1 arguments, got %" PRIu64, argc);
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
                                           ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
  }
  return argv[0]->value.as_str->length == 0 ? ARGON_FALSE : ARGON_TRUE;
}

ArgonObject *ARGON_BOOL_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
  }
  return argv[0];
}

ArgonObject *ARGON_NULL_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state) {
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
  }
  return ARGON_FALSE;
}
ArgonObject *ARGON_NULL_TYPE___number__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
  }
  return new_number_object_from_int64(0);
}
ArgonObject *ARGON_NULL_TYPE___string__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  (void)argv;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
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
  ARGON_NULL = new_instance(ARGON_NULL_TYPE);
  ARGON_NULL->type = TYPE_NULL;
  ARGON_NULL->as_bool = false;

  add_builtin_field(BASE_CLASS, __base__, ARGON_NULL);
  add_builtin_field(BASE_CLASS, __class__, ARGON_TYPE_TYPE);

  ARGON_BOOL_TYPE = new_class();
  add_builtin_field(ARGON_BOOL_TYPE, __base__, BASE_CLASS);
  ARGON_TRUE = new_instance(ARGON_BOOL_TYPE);
  ARGON_TRUE->type = TYPE_BOOL;
  ARGON_FALSE = new_instance(ARGON_BOOL_TYPE);
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
      ARGON_STRING_TYPE, __init__,
      create_argon_native_function("__init__", ARGON_STRING_TYPE___init__));
  add_builtin_field(
      ARGON_STRING_TYPE, __hash__,
      create_argon_native_function("__hash__", ARGON_STRING_TYPE___hash__));
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
  DIVIDE_FUNCTION =
      create_argon_native_function("divide", ARGON_DIVIDE_FUNCTION);
  add_builtin_field(BASE_CLASS, __getattribute__,
                    create_argon_native_function("__getattribute__",
                                                 BASE_CLASS___getattribute__));
  add_builtin_field(
      BASE_CLASS, __setattr__,
      create_argon_native_function("__setattr__", BASE_CLASS___setattr__));
  create_ARGON_DICTIONARY_TYPE();
  create_ARGON_NUMBER_TYPE();
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
  add_to_scope(Global_Scope, "string", ARGON_STRING_TYPE);
  add_to_scope(Global_Scope, "type", ARGON_TYPE_TYPE);
  add_to_scope(Global_Scope, "boolean", ARGON_BOOL_TYPE);
  add_to_scope(Global_Scope, "number", ARGON_NUMBER_TYPE);
  add_to_scope(Global_Scope, "add", ADDITION_FUNCTION);
  add_to_scope(Global_Scope, "subtract", SUBTRACTION_FUNCTION);
  add_to_scope(Global_Scope, "multiply", MULTIPLY_FUNCTION);
  add_to_scope(Global_Scope, "divide", DIVIDE_FUNCTION);
  add_to_scope(Global_Scope, "dictionary", ARGON_DICTIONARY_TYPE);

  struct hashmap_GC *argon_term = createHashmap_GC();
  add_to_hashmap(argon_term, "log",
                 create_argon_native_function("log", term_log));
  add_to_scope(Global_Scope, "term", create_dictionary(argon_term));

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

static inline void load_const(Translated *translated, RuntimeState *state) {
  uint8_t to_register = pop_byte(translated, state);
  size_t length = pop_bytecode(translated, state);
  uint64_t offset = pop_bytecode(translated, state);
  ArgonObject *object = new_string_object(
      arena_get(&translated->constants, offset), length, 0, 0);
  state->registers[to_register] = object;
}

struct hashmap *runtime_hash_table = NULL;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash) {
  if (!runtime_hash_table) {
    runtime_hash_table = createHashmap();
  } else if (prehash) {
    void *result = hashmap_lookup(runtime_hash_table, prehash);
    if (result) {
      return (uint64_t)result;
    }
  }
  uint64_t hash = siphash64_bytes(data, len, siphash_key);
  hashmap_insert(runtime_hash_table, prehash, 0, (void *)hash, 0);
  return hash;
}

static inline void load_variable(Translated *translated, RuntimeState *state,
                                 struct Stack *stack, ArErr *err) {
  int64_t length = pop_bytecode(translated, state);
  int64_t offset = pop_bytecode(translated, state);
  uint64_t prehash = pop_bytecode(translated, state);
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

RuntimeState init_runtime_state(Translated translated, char *path) {
  RuntimeState runtime = {
      ar_alloc(translated.registerCount * sizeof(ArgonObject *)),
      0,
      path,
      NULL,
      NULL,
      {0, 0, 0},
      {}};
  for (size_t i = 0; i < translated.registerCount; i++) {
    runtime.registers[i] = NULL;
  }
  return runtime;
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

void runtime(Translated _translated, RuntimeState _state, Stack *stack,
             ArErr *err) {
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
      [OP_DIVISION] = &&DO_DIVISION,
      [OP_NOT] = &&DO_NOT,
      [OP_LOAD_SETATTR_METHOD] = &&DO_LOAD_SETATTR_METHOD,
      [OP_CREATE_DICTIONARY] = &&DO_CREATE_DICTIONARY,
      [OP_LOAD_SETITEM_METHOD] = &&DO_LOAD_SETITEM_METHOD};
  _state.head = 0;

  StackFrame *currentStackFrame = ar_alloc(sizeof(StackFrame));
  *currentStackFrame = (StackFrame){_translated, _state, stack, NULL, 0};
  currentStackFrame->state.currentStackFramePointer = &currentStackFrame;
  while (currentStackFrame) {
    while (likely(currentStackFrame->state.head <
                      currentStackFrame->translated.bytecode.size &&
                  !err->exists)) {
      Translated *translated = &currentStackFrame->translated;
      RuntimeState *state = &currentStackFrame->state;
      uint8_t instruction = pop_byte(translated, state);
      // printf("instruction: %d\n", instruction);
      // for (size_t i = 0; i < translated->bytecode.size; i++) {
      //   if (i == state->head)
      //     printf("\n");
      //   printf("%d ", ((uint8_t *)translated->bytecode.data)[i]);
      // }
      // printf("\n");
      goto *dispatch_table[instruction];
    DO_LOAD_NULL:
      state->registers[pop_byte(translated, state)] = ARGON_NULL;
      continue;
    DO_LOAD_STRING:
      load_const(translated, state);
      continue;
    DO_LOAD_NUMBER:
      load_number(translated, state);
      continue;
    DO_LOAD_FUNCTION:
      load_argon_function(translated, state, currentStackFrame->stack);
      continue;
    DO_IDENTIFIER:
      load_variable(translated, state, currentStackFrame->stack, err);
      continue;
    DO_DECLARE:
      runtime_declaration(translated, state, currentStackFrame->stack, err);
      continue;
    DO_ASSIGN:
      runtime_assignment(translated, state, currentStackFrame->stack);
      continue;
    DO_BOOL: {
      if (state->registers[0] == ARGON_TRUE ||
          state->registers[0] == ARGON_FALSE)
        continue;
      if (likely(state->registers[0]->type != TYPE_OBJECT)) {
        state->registers[0] =
            state->registers[0]->as_bool ? ARGON_TRUE : ARGON_FALSE;
        continue;
      }
      ArgonObject *args[] = {ARGON_BOOL_TYPE, state->registers[0]};
      state->registers[0] = ARGON_BOOL_TYPE___new__(2, args, err, state);
      continue;
    }
    DO_JUMP_IF_FALSE: {
      uint8_t from_register = pop_byte(translated, state);
      uint64_t to = pop_bytecode(translated, state);
      if (state->registers[from_register] == ARGON_FALSE) {
        state->head = to;
      }
      continue;
    }
    DO_NOT:
      state->registers[0] =
          state->registers[0] == ARGON_FALSE ? ARGON_TRUE : ARGON_FALSE;
      continue;
    DO_JUMP:
      state->head = pop_bytecode(translated, state);
      continue;
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
      size_t length = pop_bytecode(translated, state);
      call_instance call_instance = {state->call_instance, state->registers[0],
                                     ar_alloc(length * sizeof(ArgonObject *)),
                                     length};
      state->call_instance = ar_alloc(sizeof(call_instance));
      *state->call_instance = call_instance;
      continue;
    }
    DO_INSERT_ARG:;
      size_t index = pop_bytecode(translated, state);
      state->call_instance->args[index] = state->registers[0];
      continue;
    DO_CALL: {
      run_call(state->call_instance->to_call, state->call_instance->args_length,
               state->call_instance->args, state, false, err);
      state->call_instance = (*state->call_instance).previous;
      continue;
    }
    DO_SOURCE_LOCATION:
      state->source_location = (SourceLocation){
          pop_bytecode(translated, state), pop_bytecode(translated, state),
          pop_bytecode(translated, state)};
      continue;
    DO_LOAD_BOOL:
      state->registers[0] =
          pop_byte(translated, state) ? ARGON_TRUE : ARGON_FALSE;
      continue;
    DO_LOAD_GETATTRIBUTE_METHOD:
      state->registers[0] = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __getattribute__,
          state->registers[0]);
      if (!state->registers[0]) {
        *err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __getattribute__ from objects class");
      }
      continue;
    DO_COPY_TO_REGISTER: {
      uint8_t from_register = pop_byte(translated, state);
      uint64_t to_register = pop_byte(translated, state);
      state->registers[to_register] = state->registers[from_register];
      continue;
    }
    DO_ADDITION: {
      uint8_t registerA = pop_byte(translated, state);
      uint8_t registerB = pop_byte(translated, state);
      uint8_t registerC = pop_byte(translated, state);

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (likely(valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER)) {
        if (likely(valueA->value.as_number->is_int64 &&
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
          mpq_set_si(a_GMP, a, 1);
          mpq_set_si(b_GMP, b, 1);
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
            mpq_set_si(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            mpq_set_si(b_GMP, valueB->value.as_number->n.i64, 1);
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
          ARGON_ADDITION_FUNCTION(2, args, err, state);
      continue;
    }
    DO_SUBTRACTION: {
      uint8_t registerA = pop_byte(translated, state);
      uint8_t registerB = pop_byte(translated, state);
      uint8_t registerC = pop_byte(translated, state);

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (likely(valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER)) {
        if (likely(valueA->value.as_number->is_int64 &&
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
          mpq_set_si(a_GMP, a, 1);
          mpq_set_si(b_GMP, b, 1);
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
            mpq_set_si(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            mpq_set_si(b_GMP, valueB->value.as_number->n.i64, 1);
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
          ARGON_SUBTRACTION_FUNCTION(2, args, err, state);
      continue;
    }
    DO_MULTIPLICATION: {
      uint8_t registerA = pop_byte(translated, state);
      uint8_t registerB = pop_byte(translated, state);
      uint8_t registerC = pop_byte(translated, state);

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (likely(valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER)) {
        if (likely(valueA->value.as_number->is_int64 &&
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
          mpq_set_si(a_GMP, a, 1);
          mpq_set_si(b_GMP, b, 1);
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
            mpq_set_si(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            mpq_set_si(b_GMP, valueB->value.as_number->n.i64, 1);
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
          ARGON_MULTIPLY_FUNCTION(2, args, err, state);
      continue;
    }
    DO_DIVISION: {
      uint8_t registerA = pop_byte(translated, state);
      uint8_t registerB = pop_byte(translated, state);
      uint8_t registerC = pop_byte(translated, state);

      ArgonObject *valueA = state->registers[registerA];
      ArgonObject *valueB = state->registers[registerB];

      if (likely(valueA->type == TYPE_NUMBER && valueB->type == TYPE_NUMBER)) {
        if (likely(valueA->value.as_number->is_int64 &&
                   valueB->value.as_number->is_int64)) {
          int64_t a = valueA->value.as_number->n.i64;
          int64_t b = valueB->value.as_number->n.i64;
          if (!b) {
            *err = create_err(state->source_location.line,
                              state->source_location.column,
                              state->source_location.length, state->path,
                              "Zero Division Error", "division by zero");
            continue;
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
            mpq_set_si(a_GMP, valueA->value.as_number->n.i64, 1);
            mpq_set(b_GMP, *valueB->value.as_number->n.mpq);
          } else {
            mpq_set(a_GMP, *valueA->value.as_number->n.mpq);
            if (!valueB->value.as_number->n.i64) {
              *err = create_err(state->source_location.line,
                                state->source_location.column,
                                state->source_location.length, state->path,
                                "Zero Division Error", "division by zero");
              continue;
            }
            mpq_set_si(b_GMP, valueB->value.as_number->n.i64, 1);
          }
          mpq_div(a_GMP, a_GMP, b_GMP);
          state->registers[registerC] = new_number_object(a_GMP);
          mpq_clear(a_GMP);
          mpq_clear(b_GMP);
        }
        continue;
      }

      ArgonObject *args[] = {valueA, valueB};
      state->registers[registerC] = ARGON_DIVIDE_FUNCTION(2, args, err, state);
      continue;
    }
    DO_LOAD_SETATTR_METHOD: {
      state->registers[0] = get_builtin_field_for_class(
          get_builtin_field(state->registers[0], __class__), __setattr__,
          state->registers[0]);
      if (!state->registers[0]) {
        *err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __setattr__ from objects class");
      }
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
        *err = create_err(
            state->source_location.line, state->source_location.column,
            state->source_location.length, state->path, "Runtime Error",
            "unable to get __setitem__ from objects class");
      }
      continue;
    }
    }

    ArgonObject *result = currentStackFrame->state.registers[0];
    currentStackFrame = currentStackFrame->previousStackFrame;
    if (currentStackFrame)
      currentStackFrame->state.registers[0] = result;
  }
}
