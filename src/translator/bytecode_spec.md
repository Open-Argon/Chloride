<!--
SPDX-FileCopyrightText: 2026 William Bell

SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Bytecode Specification

all opcodes are uint8_t, and all operands are uint64_t unless marked with an asterisk (*), where it is marked as uint8_t

## OP_LOAD_STRING

loads and initialises a string from the constant buffer into the provided register.

this operation 4 operands.

1. the register to write to. (*)
1. the length of the data in the constant buffer.
1. the offset in the constant buffer.

## OP_DECLARE

initilises a variable on the current scope with a given value. errors if the variable is already initilises on the current scope.

this operation takes 3 operands.

1. the length of the variable name.
1. the offset in the constant buffer of the variable name.
1. the fixed hash of the variable name.
1. the register of the given value (*)

## OP_ASSIGN

assigns to a variable in the stack. if the variable doesnt exist on the stack, it is automatically declared on the current scope.

this operation takes 3 operands.

1. the length of the variable name.
1. the offset in the constant buffer of the variable name.
1. the fixed hash of the variable name.
1. the register of the given value (*)

## OP_LOAD_NULL

sets a given register to null.

this operation takes 1 operand.

1. the register to set to null. (*)

## OP_LOAD_FUNCTION

initilises a function to a given register.

1. the offset of the name of the function.
1. the length of the name of the function.
1. the number of arguments.
1. the offset of the name of the argument.
1. the length of the name of the argument.
1. instruction 4 and 5 loop for each argument.
1. the offset of the bytecode of the function.
1. the length of the bytecode of the function.

## OP_IDENTIFIER

initilises a function to a given register.

1. the length of the identifer.
1. the offset of the identifier.
1. the fixed hash of the variable name.

## OP_BOOL

converts a value in register 0 into true or false depending on the result from \_\_bool\_\_ (using asBool if the object is a primitive)

## OP_JUMP_IF_FALSE

jumps when a the value in the given register is false.

1. the register to read. (*)
1. the index to jump to.

## OP_JUMP

jumps unconditionally to an index.

1. the index to jump to.

## OP_NEW_SCOPE

creates a new stack

## OP_EMPTY_SCOPE

empties the current scope so the same memory can be reused.

## OP_POP_SCOPE

pops the top scope off the current

## OP_INIT_CALL

initialises a call instance struct and arguments buffer.

1. the number of objects for the arguments buffer

## OP_INSERT_ARG

1. index of the argument in the arguments buffer to write the object from the register into.

## OP_CALL

call the function at the head of the call instance stack, then pops it off the stack.

## OP_SOURCE_LOCATION

sets the source location onto the runtime

1. the line
1. the column
1. the length

## OP_LOAD_GETATTRIBUTE_METHOD

loads the \_\_getattribute\_\_ method from the objects class in register 0 and put it into register 0

## OP_LOAD_BOOL

loads a boolean into register 0

1. byte representing true or false (1 or 0) *

## OP_LOAD_NUMBER

loads a mpq_t / int64 number into memory

1. the register to write to. (*)
1. is int64 (*)
1. the size of the numerator in the constant buffer.
1. the offset in the constant buffer of the numerator.
1. is integer. (*)
1. the size of the denominator in the constant buffer.
1. the offset in the constant buffer of the denominator.

## OP_COPY_TO_REGISTER

copies the value from one register to another

1. the register to copy from (*)
2. the register to write to (*)

## OP_ADDITION

performs addition between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_SUBTRACTION

performs subtraction between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_MULTIPLICATION

performs multiplication between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_EXPONENTIATION

performs exponentiation between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_DIVISION

performs division between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_FLOOR_DIVISION

performs floor division between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_MODULO

performs modulo between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_EQUAL

performs equality between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_NOT_EQUAL

performs not equality between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_LESS_THAN

performs a less than operation between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_GREATER_THAN

performs a greater than operation between register A and register B, storing the result in register C

1. the register A (*)
2. the register B (*)
2. the register C (*)

## OP_NOT

inverts the boolean value in register 0.

## OP_NEGATION

calls the \_\_negation\_\_ method on the value in register 0 and storing it in register 0.

## OP_LOAD_SETATTR_METHOD

loads the \_\_setattr\_\_ method from the objects class in register 0 and put it into register 0

## OP_CREATE_DICTIONARY

create a dictionary object into register 0.

## OP_LOAD_GETITEM_METHOD

loads the \_\_getitem\_\_ method from the objects class in register 0 and put it into register 0

## OP_LOAD_SETITEM_METHOD

loads the \_\_setitem\_\_ method from the objects class in register 0 and put it into register 0