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
#include "access/access.h"
#include "call/call.h"
#include "declaration/declaration.h"
#include "internals/hashmap/hashmap.h"
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

ArgonObject *ARGON_METHOD_TYPE;
Stack *Global_Scope = NULL;
ArgonObject *ACCESS_FUNCTION;
ArgonObject *ADDITION_FUNCTION;
ArgonObject *SUBTRACTION_FUNCTION;
ArgonObject *MULTIPLY_FUNCTION;
ArgonObject *DIVISION_FUNCTION;
ArgonObject *POWER_FUNCTION;
ArgonObject *MODULO_FUNCTION;
ArgonObject *FLOORDIVISION_FUNCTION;

ArgonObject *ARGON_ADDITION_FUNCTION(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "add expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *__add__ = get_field_for_class(
        get_field(output, "__class__", false, false), "__add__", output);
    if (!__add__) {
      ArgonObject *cls___name__ = get_field(output, "__name__", true, false);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __add__ method",
                        (int)cls___name__->value.as_str.length,
                        cls___name__->value.as_str.data);
      return ARGON_NULL;
    }
    output = argon_call(__add__, 1, (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_SUBTRACTION_FUNCTION(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "subtract expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *__subtract__ = get_field_for_class(
        get_field(output, "__class__", false, false), "__subtract__", output);
    if (!__subtract__) {
      ArgonObject *cls___name__ = get_field(output, "__name__", true, false);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __subtract__ method",
                        (int)cls___name__->value.as_str.length,
                        cls___name__->value.as_str.data);
      return ARGON_NULL;
    }
    output =
        argon_call(__subtract__, 1, (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_MULTIPLY_FUNCTION(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "multiply expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *__multiply__ = get_field_for_class(
        get_field(output, "__class__", false, false), "__multiply__", output);
    if (!__multiply__) {
      ArgonObject *cls___name__ = get_field(output, "__name__", true, false);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __multiply__ method",
                        (int)cls___name__->value.as_str.length,
                        cls___name__->value.as_str.data);
      return ARGON_NULL;
    }
    output =
        argon_call(__multiply__, 1, (ArgonObject *[]){argv[i]}, err, state);
  }
  return output;
}

ArgonObject *ARGON_DIVISION_FUNCTION(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state) {
  if (argc < 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "division expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *output = argv[0];
  for (size_t i = 1; i < argc; i++) {
    ArgonObject *__multiply__ = get_field_for_class(
        get_field(output, "__class__", false, false), "__division__", output);
    if (!__multiply__) {
      ArgonObject *cls___name__ = get_field(output, "__name__", true, false);
      *err = create_err(0, 0, 0, "", "Runtime Error",
                        "Object '%.*s' is missing __division__ method",
                        (int)cls___name__->value.as_str.length,
                        cls___name__->value.as_str.data);
      return ARGON_NULL;
    }
    output =
        argon_call(__multiply__, 1, (ArgonObject *[]){argv[i]}, err, state);
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
    ArgonObject *cls_class = get_field(argv[1], "__class__", true, false);
    if (cls_class)
      return cls_class;
    return ARGON_NULL;
  }
  ArgonObject *cls___new__ = get_field_for_class(argv[0], "__new__", NULL);
  if (!cls___new__) {
    ArgonObject *cls___name__ = get_field(argv[0], "__name__", true, false);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "Object '%.*s' is missing __new__ method, so cannot be initialised",
        (int)cls___name__->value.as_str.length,
        cls___name__->value.as_str.data);
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
        get_field_for_class(argv[0], "__init__", new_object);
    if (!cls___init__) {
      ArgonObject *cls___name__ = get_field(argv[0], "__name__", true, false);
      *err = create_err(
          0, 0, 0, "", "Runtime Error",
          "Object '%.*s' is missing __init__ method, so cannot be initialised",
          (int)cls___name__->value.as_str.length,
          cls___name__->value.as_str.data);
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
  ArgonObject *new_obj = new_object();
  add_field(new_obj, "__class__", argv[0]);
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

ArgonObject *BASE_CLASS___string__(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
  }

  ArgonObject *object_name = get_field_for_class(argv[0], "__name__", NULL);
  ArgonObject *class_name = get_field_for_class(
      get_field(argv[0], "__class__", false, false), "__name__", NULL);

  char buffer[100];
  if (class_name && object_name)
    snprintf(buffer, sizeof(buffer), "<%.*s %.*s at %p>",
             (int)class_name->value.as_str.length,
             class_name->value.as_str.data,
             (int)object_name->value.as_str.length,
             object_name->value.as_str.data, argv[0]);
  else
    snprintf(buffer, sizeof(buffer), "<object at %p>", argv[0]);
  return new_string_object_null_terminated(buffer);
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

  self->value.as_str.data = NULL;
  self->value.as_str.length = 0;
  self->type = TYPE_STRING;
  ArgonObject *string_convert_method = get_field_for_class(
      get_field(object, "__class__", false, false), "__string__", object);
  if (string_convert_method) {
    ArgonObject *string_object =
        argon_call(string_convert_method, 0, NULL, err, state);
    if (err->exists)
      return ARGON_NULL;
    self->value.as_str.data =
        ar_alloc_atomic(string_object->value.as_str.length);
    memcpy(self->value.as_str.data, string_object->value.as_str.data,
           string_object->value.as_str.length);
    self->value.as_str.length = string_object->value.as_str.length;
  }
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
  ArgonObject *boolean_convert_method = get_field_for_class(
      get_field(object, "__class__", false, false), "__boolean__", object);
  if (boolean_convert_method) {
    ArgonObject *boolean_object =
        argon_call(boolean_convert_method, 0, NULL, err, state);
    if (err->exists)
      return ARGON_NULL;
    return boolean_object;
  }
  ArgonObject *type_name = get_field_for_class(
      get_field(object, "__class__", false, false), "__name__", object);
  *err = create_err(
      0, 0, 0, "", "Runtime Error", "cannot convert type '%.*s' to bool",
      type_name->value.as_str.length, type_name->value.as_str.data);
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
    ArgonObject *type_name = get_field_for_class(
        get_field(argv[1], "__class__", false, false), "__name__", argv[1]);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__add__ cannot perform concatenation between a string and %.*s",
                      type_name->value.as_str.length,
                      type_name->value.as_str.data);
    return ARGON_NULL;
  }
  size_t length = argv[0]->value.as_str.length+argv[1]->value.as_str.length;
  char*concat = malloc(length);
  memcpy(concat,argv[0]->value.as_str.data, argv[0]->value.as_str.length);
  memcpy(concat+argv[0]->value.as_str.length,argv[1]->value.as_str.data, argv[1]->value.as_str.length);
  ArgonObject* object = new_string_object(concat, length);
  free(concat);
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
  return new_number_object_from_long(argv[0] == ARGON_TRUE, 1);
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
  int result = mpq_set_decimal_str_exp(r, argv[0]->value.as_str.data,
                                       argv[0]->value.as_str.length);
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
  return argv[0]->value.as_str.length == 0 ? ARGON_FALSE : ARGON_TRUE;
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
  return new_number_object_from_long(0, 1);
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
  BASE_CLASS = new_object();
  ARGON_TYPE_TYPE = new_object();
  add_field(ARGON_TYPE_TYPE, "__base__", BASE_CLASS);
  add_field(ARGON_TYPE_TYPE, "__class__", ARGON_TYPE_TYPE);

  ARGON_NULL_TYPE = new_object();
  add_field(ARGON_NULL_TYPE, "__base__", BASE_CLASS);
  ARGON_NULL = new_object();
  add_field(ARGON_NULL, "__class__", ARGON_NULL_TYPE);

  add_field(BASE_CLASS, "__base__", NULL);
  add_field(BASE_CLASS, "__class__", ARGON_TYPE_TYPE);

  ARGON_BOOL_TYPE = new_object();
  add_field(ARGON_BOOL_TYPE, "__base__", BASE_CLASS);
  ARGON_TRUE = new_object();
  add_field(ARGON_TRUE, "__class__", ARGON_BOOL_TYPE);
  ARGON_FALSE = new_object();
  add_field(ARGON_FALSE, "__class__", ARGON_BOOL_TYPE);

  ARGON_STRING_TYPE = new_object();
  add_field(ARGON_STRING_TYPE, "__base__", BASE_CLASS);

  add_field(ARGON_STRING_TYPE, "__name__",
            new_string_object_null_terminated("string"));
  add_field(BASE_CLASS, "__name__",
            new_string_object_null_terminated("object"));
  add_field(ARGON_TYPE_TYPE, "__name__",
            new_string_object_null_terminated("type"));
  add_field(ARGON_NULL_TYPE, "__name__",
            new_string_object_null_terminated("null_type"));
  add_field(ARGON_BOOL_TYPE, "__name__",
            new_string_object_null_terminated("boolean"));

  ARGON_FUNCTION_TYPE = new_object();
  add_field(ARGON_FUNCTION_TYPE, "__base__", BASE_CLASS);
  add_field(ARGON_FUNCTION_TYPE, "__name__",
            new_string_object_null_terminated("function"));

  ARGON_METHOD_TYPE = new_object();
  add_field(ARGON_METHOD_TYPE, "__base__", BASE_CLASS);
  add_field(ARGON_METHOD_TYPE, "__name__",
            new_string_object_null_terminated("method"));
  create_ARGON_NUMBER_TYPE();

  add_field(BASE_CLASS, "__new__",
            create_argon_native_function("__new__", BASE_CLASS___new__));
  add_field(BASE_CLASS, "address",
            create_argon_native_function("address", BASE_CLASS_address));
  add_field(BASE_CLASS, "__init__",
            create_argon_native_function("__init__", BASE_CLASS___new__));
  add_field(BASE_CLASS, "__string__",
            create_argon_native_function("__string__", BASE_CLASS___string__));
  add_field(ARGON_TYPE_TYPE, "__call__",
            create_argon_native_function("__call__", ARGON_TYPE_TYPE___call__));
  add_field(
      ARGON_STRING_TYPE, "__init__",
      create_argon_native_function("__init__", ARGON_STRING_TYPE___init__));
  add_field(
      ARGON_STRING_TYPE, "__add__",
      create_argon_native_function("__add__", ARGON_STRING_TYPE___add__));
  add_field(
      ARGON_STRING_TYPE, "__number__",
      create_argon_native_function("__number__", ARGON_STRING_TYPE___number__));
  add_field(
      ARGON_NULL_TYPE, "__boolean__",
      create_argon_native_function("__boolean__", ARGON_NULL_TYPE___boolean__));
  add_field(
      ARGON_NULL_TYPE, "__string__",
      create_argon_native_function("__string__", ARGON_NULL_TYPE___string__));
  add_field(
      ARGON_NULL_TYPE, "__number__",
      create_argon_native_function("__number__", ARGON_NULL_TYPE___number__));
  add_field(
      ARGON_STRING_TYPE, "__string__",
      create_argon_native_function("__string__", ARGON_STRING_TYPE___string__));
  add_field(ARGON_BOOL_TYPE, "__new__",
            create_argon_native_function("__new__", ARGON_BOOL_TYPE___new__));
  add_field(
      ARGON_BOOL_TYPE, "__boolean__",
      create_argon_native_function("__boolean__", ARGON_BOOL_TYPE___boolean__));
  add_field(ARGON_STRING_TYPE, "__boolean__",
            create_argon_native_function("__boolean__",
                                         ARGON_STRING_TYPE___boolean__));
  add_field(
      BASE_CLASS, "__boolean__",
      create_argon_native_function("__boolean__", BASE_CLASS___boolean__));
  add_field(
      ARGON_BOOL_TYPE, "__string__",
      create_argon_native_function("__string__", ARGON_BOOL_TYPE___string__));
  add_field(
      ARGON_BOOL_TYPE, "__number__",
      create_argon_native_function("__number__", ARGON_BOOL_TYPE___number__));
  ACCESS_FUNCTION = create_argon_native_function("__get_attr__",
                                                 ARGON_TYPE_TYPE___get_attr__);
  ADDITION_FUNCTION =
      create_argon_native_function("add", ARGON_ADDITION_FUNCTION);
  SUBTRACTION_FUNCTION =
      create_argon_native_function("subtract", ARGON_SUBTRACTION_FUNCTION);
  MULTIPLY_FUNCTION =
      create_argon_native_function("multiply", ARGON_MULTIPLY_FUNCTION);
  DIVISION_FUNCTION =
      create_argon_native_function("division", ARGON_DIVISION_FUNCTION);
  add_field(BASE_CLASS, "__get_attr__", ACCESS_FUNCTION);
}

void add_to_scope(Stack *stack, char *name, ArgonObject *value) {
  size_t length = strlen(name);
  uint64_t hash = siphash64_bytes(name, length, siphash_key);
  ArgonObject *key = new_string_object(name, length);
  hashmap_insert_GC(stack->scope, hash, key, value, 0);
}

void bootstrap_globals() {
  Global_Scope = create_scope(NULL);
  add_to_scope(Global_Scope, "string", ARGON_STRING_TYPE);
  add_to_scope(Global_Scope, "type", ARGON_TYPE_TYPE);
  add_to_scope(Global_Scope, "boolean", ARGON_BOOL_TYPE);
  add_to_scope(Global_Scope, "number", ARGON_NUMBER_TYPE);
  add_to_scope(Global_Scope, "add", ADDITION_FUNCTION);
  add_to_scope(Global_Scope, "subtract", SUBTRACTION_FUNCTION);
  add_to_scope(Global_Scope, "multiply", MULTIPLY_FUNCTION);
  add_to_scope(Global_Scope, "division", DIVISION_FUNCTION);

  ArgonObject *argon_term = new_object();
  add_field(argon_term, "__init__", ARGON_NULL);
  add_field(argon_term, "log", create_argon_native_function("log", term_log));
  add_to_scope(Global_Scope, "term", argon_term);
}

int compare_by_order(const void *a, const void *b) {
  const struct node_GC *itemA = (const struct node_GC *)a;
  const struct node_GC *itemB = (const struct node_GC *)b;
  return itemA->order - itemB->order;
}

uint8_t pop_byte(Translated *translated, RuntimeState *state) {
  return *((uint8_t *)darray_get(&translated->bytecode, state->head++));
}

uint64_t pop_bytecode(Translated *translated, RuntimeState *state) {
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= ((uint64_t)pop_byte(translated, state)) << (i * 8);
  }
  return value;
}

void load_const(Translated *translated, RuntimeState *state) {
  uint64_t to_register = pop_byte(translated, state);
  size_t length = pop_bytecode(translated, state);
  uint64_t offset = pop_bytecode(translated, state);

  void *data = ar_alloc_atomic(length);
  memcpy(data, arena_get(&translated->constants, offset), length);
  ArgonObject *object = new_string_object(data, length);
  state->registers[to_register] = object;
}

struct hashmap *runtime_hash_table = NULL;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash) {
  if (!runtime_hash_table) {
    runtime_hash_table = createHashmap();
  } else {
    void *result = hashmap_lookup(runtime_hash_table, prehash);
    if (result) {
      return (uint64_t)result;
    }
  }
  uint64_t hash = siphash64_bytes(data, len, siphash_key);
  hashmap_insert(runtime_hash_table, prehash, 0, (void *)hash, 0);
  return hash;
}

ArErr load_variable(Translated *translated, RuntimeState *state,
                    struct Stack *stack) {
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
      return no_err;
    }
    current_stack = current_stack->prev;
  }
  ArErr err =
      create_err(state->source_location.line, state->source_location.column,
                 state->source_location.length, state->path, "Name Error",
                 "name '%.*s' is not defined", (int)length,
                 arena_get(&translated->constants, offset));
  return err;
}

ArErr run_instruction(Translated *translated, RuntimeState *state,
                      struct Stack **stack) {
  OperationType opcode = pop_byte(translated, state);
  switch (opcode) {
  case OP_LOAD_NULL:
    state->registers[pop_byte(translated, state)] = ARGON_NULL;
    break;
  case OP_LOAD_STRING:
    load_const(translated, state);
    break;
  case OP_LOAD_NUMBER:
    load_number(translated, state);
    break;
  case OP_LOAD_FUNCTION:
    load_argon_function(translated, state, *stack);
    break;
  case OP_IDENTIFIER:
    return load_variable(translated, state, *stack);
  case OP_DECLARE:
    return runtime_declaration(translated, state, *stack);
  case OP_BOOL:;
    ArErr err = no_err;
    uint8_t to_register = pop_byte(translated, state);
    ArgonObject *args[] = {ARGON_BOOL_TYPE, state->registers[0]};
    state->registers[to_register] =
        ARGON_BOOL_TYPE___new__(2, args, &err, state);
    return err;
  case OP_JUMP_IF_FALSE:;
    uint8_t from_register = pop_byte(translated, state);
    uint64_t to = pop_bytecode(translated, state);
    if (state->registers[from_register] == ARGON_FALSE) {
      state->head = to;
    }
    break;
  case OP_JUMP:
    state->head = pop_bytecode(translated, state);
    break;
  case OP_NEW_SCOPE:
    *stack = create_scope(*stack);
    break;
  case OP_POP_SCOPE:;
    // struct node_GC *array =
    //     checked_malloc(sizeof(struct node_GC) * (*stack)->scope->count);
    // size_t j = 0;
    // for (size_t i = 0; i < (*stack)->scope->size; i++) {
    //   struct node_GC *temp = (*stack)->scope->list[i];
    //   while (temp) {
    //     array[j++] = *temp;
    //     temp = temp->next;
    //   }
    // }
    // qsort(array, (*stack)->scope->count, sizeof(struct node_GC),
    //       compare_by_order);
    // for (size_t i = 0; i < (*stack)->scope->count; i++) {
    //   struct node_GC temp = array[i];
    //   printf("%.*s = %.*s\n",
    //            (int)((ArgonObject *)temp.key)->value.as_str.length,
    //            ((ArgonObject *)temp.key)->value.as_str.data,
    //            (int)((ArgonObject *)temp.val)->value.as_str.length,
    //            ((ArgonObject *)temp.val)->value.as_str.data);
    // }
    // free(array);
    *stack = (*stack)->prev;
    break;
  case OP_INIT_CALL:;
    size_t length = pop_bytecode(translated, state);
    call_instance call_instance = {state->call_instance, state->registers[0],
                                   ar_alloc(length * sizeof(ArgonObject *)),
                                   length};
    state->call_instance = ar_alloc(sizeof(call_instance));
    *state->call_instance = call_instance;
    break;
  case OP_INSERT_ARG:;
    size_t index = pop_bytecode(translated, state);
    (state->call_instance->args)[index] = state->registers[0];
    break;
  case OP_CALL:;
    err = run_call(state->call_instance->to_call,
                   state->call_instance->args_length,
                   state->call_instance->args, state, false);
    state->call_instance = (*state->call_instance).previous;
    return err;
    // ArgonObject *object = state->registers[from_register];
    // char *field = "__class__";
    // uint64_t hash = siphash64_bytes(field, strlen(field), siphash_key);
    // ArgonObject *class = hashmap_lookup_GC(object->dict, hash);
    // field = "__name__";
    // hash = siphash64_bytes(field, strlen(field), siphash_key);
    // ArgonObject *class_name = hashmap_lookup_GC(class->dict, hash);
    // hash = siphash64_bytes(field, strlen(field), siphash_key);
    // ArgonObject *object_name = hashmap_lookup_GC(object->dict, hash);
    // if (object_name) {
    //   printf("call <%.*s %.*s at %p>\n",
    //   (int)class_name->value.as_str.length,
    //   class_name->value.as_str.data,(int)object_name->value.as_str.length,
    //   object_name->value.as_str.data, object);
    // } else {
    //   printf("call <%.*s object at %p>\n",
    //   (int)class_name->value.as_str.length, class_name->value.as_str.data,
    //   object);
    // }
  case OP_SOURCE_LOCATION:
    state->source_location = (SourceLocation){pop_bytecode(translated, state),
                                              pop_bytecode(translated, state),
                                              pop_bytecode(translated, state)};
    break;
  case OP_LOAD_BOOL:
    state->registers[0] =
        pop_byte(translated, state) ? ARGON_TRUE : ARGON_FALSE;
    break;
  case OP_LOAD_ACCESS_FUNCTION:
    state->registers[0] = ACCESS_FUNCTION;
    break;
  case OP_LOAD_ADDITION_FUNCTION:
    state->registers[0] = ADDITION_FUNCTION;
    break;
  case OP_LOAD_SUBTRACTION_FUNCTION:
    state->registers[0] = SUBTRACTION_FUNCTION;
    break;
  case OP_LOAD_MULTIPLY_FUNCTION:
    state->registers[0] = MULTIPLY_FUNCTION;
    break;
  case OP_LOAD_DIVISION_FUNCTION:
    state->registers[0] = DIVISION_FUNCTION;
    break;
  default:
    return create_err(0, 0, 0, NULL, "Runtime Error", "Invalid Opcode %#x",
                      opcode);
  }
  return no_err;
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
  return runtime;
}

Stack *create_scope(Stack *prev) {
  Stack *stack = ar_alloc(sizeof(Stack));
  stack->scope = createHashmap_GC();
  stack->prev = prev;
  return stack;
}

ArErr runtime(Translated translated, RuntimeState state, Stack *stack) {
  state.head = 0;

  StackFrame *currentStackFrame =
      ar_alloc(sizeof(StackFrame) * STACKFRAME_CHUNKS);
  *currentStackFrame = (StackFrame){translated, state, stack, NULL, 0};
  currentStackFrame->state.currentStackFramePointer = &currentStackFrame;
  ArErr err = no_err;
  while (currentStackFrame) {
    while (currentStackFrame->state.head <
               currentStackFrame->translated.bytecode.size &&
           !err.exists) {
      err =
          run_instruction(&currentStackFrame->translated,
                          &currentStackFrame->state, &currentStackFrame->stack);
    }
    currentStackFrame = currentStackFrame->previousStackFrame;
  }
  return err;
}