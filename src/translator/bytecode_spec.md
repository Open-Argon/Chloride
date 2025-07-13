# Bytecode Specification

all opcodes are uint8_t, and all operands are uint64_t unless marked with an asterisk (*), where it is marked as uint8_t

## OP_LOAD_CONST

loads and initialises a value from the constant buffer into the provided register.

this operation 4 operands.

1. the register to write to. (*)
2. the type of data from the constant buffer. (*)
3. the length of the data in the constant buffer.
4. the offset in the constant buffer.

## OP_DECLARE

initilises a variable on the current scope with a given value. errors if the variable is already initilises on the current scope.

this operation takes 3 operands.

1. the length of the variable name.
2. the offset in the constant buffer of the variable name.
3. the fixed hash of the variable name.
4. the register of the given value (*)
5. the index of the source location.

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
4. the index of the source location.

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