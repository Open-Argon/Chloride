# Bytecode Specification

## OP_LOAD_CONST

loads and initialises a value from the constant buffer into the provided register.

this operation 4 operands.

1. the register to write to.
2. the type of data from the constant buffer.
3. the length of the data in the constant buffer.
4. the offset in the constant buffer.

## OP_DECLARE

initilises a variable on the current scope with a given value. errors if the variable is already initilises on the current scope.

this operation takes 3 operands.

1. the length of the variable name.
2. the offset in the constant buffer of the variable name.
3. the register of the given value

## OP_LOAD_NULL

sets a given register to null.

this operation takes 1 operand.

1. the register to set to null.
