/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EXCEPTIONS
#define EXCEPTIONS
#include "../../../arobject.h"

extern ArgonObject *BaseException;

extern ArgonObject *Exception;

extern ArgonObject *RuntimeError;

extern ArgonObject *SyntaxError;

extern ArgonObject *ConversionError;

extern ArgonObject *MathsError;

extern ArgonObject *ZeroDivisionError;

extern ArgonObject *NameError;

extern ArgonObject *TypeError;

extern ArgonObject *InternalError;

extern ArgonObject *IndexError;

extern ArgonObject *AttributeError;

extern ArgonObject *PathError;

extern ArgonObject *FileError;

extern ArgonObject *ImportError;

extern ArgonObject *SignalException;

extern ArgonObject *KeyboardInterrupt;

extern ArgonObject *KeyboardInterrupt_instance;

extern ArgonObject *StopIteration;

extern ArgonObject *StopIteration_instance;

void init_exceptions();

#endif // EXCEPTIONS