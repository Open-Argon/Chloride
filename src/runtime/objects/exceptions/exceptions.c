/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "exceptions.h"
#include "../../../err.h"
#include "../../api/api.h"
#include "../array/array.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../object.h"
#include "../string/string.h"
#include <inttypes.h>

ArgonObject *BaseException;

ArgonObject *Exception;
ArgonObject *RuntimeError;
ArgonObject *SyntaxError;
ArgonObject *ConversionError;
ArgonObject *MathsError;
ArgonObject *ZeroDivisionError;
ArgonObject *NameError;
ArgonObject *TypeError;
ArgonObject *InternalError;
ArgonObject *IndexError;
ArgonObject *AttributeError;
ArgonObject *PathError;
ArgonObject *FileError;
ArgonObject *ImportError;

ArgonObject *SytemException;
ArgonObject *KeyboardInterrupt;

ArgonObject *KeyboardInterrupt_instance;

ArgonObject *BaseException___new__(size_t argc, ArgonObject **argv, ArErr *err,
                                   RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 2) {
    *err =
        create_err(RuntimeError,
                   "__new__ expects at least 2 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj = new_instance(argv[0], 0);
  add_builtin_field(new_obj, message, argv[1]);

  add_builtin_field(new_obj, stack_trace,
                    ARRAY_CREATE(0, NULL, NULL, NULL, NULL));
  return new_obj;
}

void init_exceptions() {
  BaseException = new_class();
  add_builtin_field(BaseException, __name__,
                    new_string_object_null_terminated("BaseException"));
  add_builtin_field(
      BaseException, __new__,
      create_argon_native_function("__new__", BaseException___new__));
  native_api.BaseException = BaseException;

  // normal errors
  Exception = new_class();
  add_builtin_field(Exception, __base__, BaseException);
  add_builtin_field(Exception, __name__,
                    new_string_object_null_terminated("Exception"));

  RuntimeError = new_class();
  add_builtin_field(RuntimeError, __base__, Exception);
  add_builtin_field(RuntimeError, __name__,
                    new_string_object_null_terminated("RuntimeError"));

  SyntaxError = new_class();
  add_builtin_field(SyntaxError, __base__, Exception);
  add_builtin_field(SyntaxError, __name__,
                    new_string_object_null_terminated("SyntaxError"));

  ConversionError = new_class();
  add_builtin_field(ConversionError, __base__, Exception);
  add_builtin_field(ConversionError, __name__,
                    new_string_object_null_terminated("ConversionError"));

  MathsError = new_class();
  add_builtin_field(MathsError, __base__, Exception);
  add_builtin_field(MathsError, __name__,
                    new_string_object_null_terminated("MathsError"));

  ZeroDivisionError = new_class();
  add_builtin_field(ZeroDivisionError, __base__, MathsError);
  add_builtin_field(ZeroDivisionError, __name__,
                    new_string_object_null_terminated("ZeroDivisionError"));

  NameError = new_class();
  add_builtin_field(NameError, __base__, Exception);
  add_builtin_field(NameError, __name__,
                    new_string_object_null_terminated("NameError"));

  AttributeError = new_class();
  add_builtin_field(AttributeError, __base__, NameError);
  add_builtin_field(AttributeError, __name__,
                    new_string_object_null_terminated("AttributeError"));

  TypeError = new_class();
  add_builtin_field(TypeError, __base__, Exception);
  add_builtin_field(TypeError, __name__,
                    new_string_object_null_terminated("TypeError"));

  InternalError = new_class();
  add_builtin_field(InternalError, __base__, Exception);
  add_builtin_field(InternalError, __name__,
                    new_string_object_null_terminated("InternalError"));

  IndexError = new_class();
  add_builtin_field(IndexError, __base__, Exception);
  add_builtin_field(IndexError, __name__,
                    new_string_object_null_terminated("IndexError"));

  PathError = new_class();
  add_builtin_field(PathError, __base__, Exception);
  add_builtin_field(PathError, __name__,
                    new_string_object_null_terminated("PathError"));

  FileError = new_class();
  add_builtin_field(FileError, __base__, Exception);
  add_builtin_field(FileError, __name__,
                    new_string_object_null_terminated("FileError"));

  ImportError = new_class();
  add_builtin_field(ImportError, __base__, Exception);
  add_builtin_field(ImportError, __name__,
                    new_string_object_null_terminated("ImportError"));

  native_api.Exception = Exception;
  native_api.RuntimeError = RuntimeError;
  native_api.SyntaxError = SyntaxError;
  native_api.ConversionError = ConversionError;
  native_api.MathsError = MathsError;
  native_api.ZeroDivisionError = ZeroDivisionError;
  native_api.NameError = NameError;
  native_api.TypeError = TypeError;
  native_api.InternalError = InternalError;
  native_api.IndexError = IndexError;
  native_api.AttributeError = AttributeError;
  native_api.PathError = PathError;
  native_api.FileError = FileError;
  native_api.ImportError = ImportError;

  // system errors
  SytemException = new_class();
  add_builtin_field(SytemException, __base__, BaseException);
  add_builtin_field(SytemException, __name__,
                    new_string_object_null_terminated("SytemException"));
  KeyboardInterrupt = new_class();
  add_builtin_field(KeyboardInterrupt, __base__, SytemException);
  add_builtin_field(KeyboardInterrupt, __name__,
                    new_string_object_null_terminated("KeyboardInterrupt"));


  KeyboardInterrupt_instance = new_instance(KeyboardInterrupt, 0);

  native_api.SytemException = SytemException;
  native_api.KeyboardInterrupt = KeyboardInterrupt;
}