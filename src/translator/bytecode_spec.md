<!--
SPDX-FileCopyrightText: 2025 William Bell

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Bytecode Specification

all opcodes are uint8_t, and all operands are uint64_t unless marked with an asterisk (*), where it is marked as uint8_t

## OP_LOAD_STRING

loads and initialises a string from the constant buffer into the provided register.

this operation 4 operands.

1. the register to write to. (*)
3. the length of the data in the constant buffer.
4. the offset in the constant buffer.

## OP_DECLARE

initilises a variable on the current scope with a given value. errors if the variable is already initilises on the current scope.

this operation takes 3 operands.

1. the length of the variable name.
2. the offset in the constant buffer of the variable name.
3. the fixed hash of the variable name.
4. the register of the given value (*)

## OP_LOAD_NULL

sets a given register to null.

this operation takes 1 operand.

1. the register to set to null. (*)

## OP_LOAD_FUNCTION

initilises a function to a given register.

1. the offset of the name of the function.
2. the length of the name of the function.
3. the number of arguments.
4. the offset of the name of the argument.
5. the length of the name of the argument.
6. instruction 4 and 5 loop for each argument.
7. the offset of the bytecode of the function.
8. the length of the bytecode of the function.

## OP_IDENTIFIER

initilises a function to a given register.

1. the length of the identifer.
2. the offset of the identifier.
3. the fixed hash of the variable name.

## OP_BOOL

converts a value in a given register into true or false depending on the result from \_\_bool\_\_

1. the register to read and write to. (*)

## OP_JUMP_IF_FALSE

jumps when a the value in the given register is false.

1. the register to read. (*)
1. the index to jump to.

## OP_JUMP_IF_FALSE

jumps unconditionally to an index.

1. the index to jump to.


## OP_NEW_SCOPE

creates a new stack

## OP_POP_SCOPE

pops the top scope off the current

## OP_INIT_CALL

initialises a call instance struct and arguments buffer.

1. the number of objects for the arguments buffer

## OP_INSERT_ARG

1. index of the argument in the arguments buffer to write the object from the register into.

## OP_CALL

call a function with args

## OP_SOURCE_LOCATION

sets the source location onto the runtime

3. the line
4. the column
5. the length

## OP_LOAD_ACCESS_FUNCTION

loads the access function into register 1

## OP_LOAD_BOOL

loads a boolean into register 1

1. byte representing true or false (1 or 0) *

## OP_LOAD_NUMBER

loads a mpq_t number into memory

1. the register to write to. (*)
3. the size of the numerator in the constant buffer.
4. the offset in the constant buffer of the numerator.
3. the size of the denominator in the constant buffer.
4. the offset in the constant buffer of the denominator.

## OP_SWAP_REGISTERS

swap the contents in two registers

1. register a (*)
2. register b (*)