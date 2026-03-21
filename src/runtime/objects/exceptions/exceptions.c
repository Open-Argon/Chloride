/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "exceptions.h"
#include "../object.h"
#include "../string/string.h"

ArgonObject *BaseException;

ArgonObject *Exception;

ArgonObject *RuntimeError;

ArgonObject *SyntaxError;

ArgonObject *ConversionError;

ArgonObject *MathsError;

ArgonObject *ZeroDivisionError;

ArgonObject *NameError;

ArgonObject *TypeError;

ArgonObject* InternalError;

ArgonObject* IndexError;

ArgonObject* AttributeError;

ArgonObject* PathError;

ArgonObject* FileError;

ArgonObject* ImportError;


void init_exceptions() {
  BaseException = new_class();
  add_builtin_field(BaseException, __name__,
                    new_string_object_null_terminated("BaseException"));
                    
  Exception = new_class();
  add_builtin_field(Exception, __class__, BaseException);
  add_builtin_field(Exception, __name__,
                    new_string_object_null_terminated("Exception"));

  RuntimeError = new_class();
  add_builtin_field(RuntimeError, __class__, Exception);
  add_builtin_field(RuntimeError, __name__,
                    new_string_object_null_terminated("RuntimeError"));

  SyntaxError = new_class();
  add_builtin_field(SyntaxError, __class__, Exception);
  add_builtin_field(SyntaxError, __name__,
                    new_string_object_null_terminated("SyntaxError"));

  ConversionError = new_class();
  add_builtin_field(ConversionError, __class__, Exception);
  add_builtin_field(ConversionError, __name__,
                    new_string_object_null_terminated("ConversionError"));

  MathsError = new_class();
  add_builtin_field(MathsError, __class__, Exception);
  add_builtin_field(MathsError, __name__,
                    new_string_object_null_terminated("MathsError"));

  ZeroDivisionError = new_class();
  add_builtin_field(ZeroDivisionError, __class__, MathsError);
  add_builtin_field(ZeroDivisionError, __name__,
                    new_string_object_null_terminated("ZeroDivisionError"));

  NameError = new_class();
  add_builtin_field(NameError, __class__, Exception);
  add_builtin_field(NameError, __name__,
                    new_string_object_null_terminated("NameError"));

  AttributeError = new_class();
  add_builtin_field(AttributeError, __class__, NameError);
  add_builtin_field(AttributeError, __name__,
                    new_string_object_null_terminated("AttributeError"));

  TypeError = new_class();
  add_builtin_field(TypeError, __class__, Exception);
  add_builtin_field(TypeError, __name__,
                    new_string_object_null_terminated("TypeError"));

  InternalError = new_class();
  add_builtin_field(InternalError, __class__, Exception);
  add_builtin_field(InternalError, __name__,
                    new_string_object_null_terminated("InternalError"));

  IndexError = new_class();
  add_builtin_field(IndexError, __class__, Exception);
  add_builtin_field(IndexError, __name__,
                    new_string_object_null_terminated("IndexError"));

  PathError = new_class();
  add_builtin_field(PathError, __class__, Exception);
  add_builtin_field(PathError, __name__,
                    new_string_object_null_terminated("PathError"));

  FileError = new_class();
  add_builtin_field(FileError, __class__, Exception);
  add_builtin_field(FileError, __name__,
                    new_string_object_null_terminated("FileError"));

  ImportError = new_class();
  add_builtin_field(ImportError, __class__, Exception);
  add_builtin_field(ImportError, __name__,
                    new_string_object_null_terminated("ImportError"));

}