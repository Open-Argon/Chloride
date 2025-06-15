import struct
from enum import Enum, EnumMeta, auto

class AutoEnumMeta(EnumMeta):
    @classmethod
    def __prepare__(metacls, clsname, bases, **kwargs):
        d = super().__prepare__(clsname, bases, **kwargs)
        d['_next_value'] = 254
        return d

    def _generate_next_value_(cls, name, start, count, last_values):
        value = cls._next_value
        cls._next_value += 1
        return value

class OperationType(Enum, metaclass=AutoEnumMeta):
    OP_LOAD_CONST = auto()
    OP_DECLARE = auto()
    OP_LOAD_NULL = auto()
    OP_JUMP = auto()

class Types(Enum, metaclass=AutoEnumMeta):
    TYPE_OP_STRING = auto()
    TYPE_OP_NUMBER = auto()

def read_arbin(filename):
    with open(filename, "rb") as f:
        # Read and verify file identifier (4 bytes)
        file_id = f.read(4)
        if file_id != b"ARBI":
            raise ValueError("Invalid file identifier")

        # Read version number (uint64_t, little-endian)
        version_number, = struct.unpack("<Q", f.read(8))

        # Read regCount, constantsSize, bytecodeSize (all uint64_t, little-endian)
        reg_count, = struct.unpack("<Q", f.read(8))
        constants_size, = struct.unpack("<Q", f.read(8))
        bytecode_size, = struct.unpack("<Q", f.read(8))

        # Read constants buffer (raw bytes)
        constants = f.read(constants_size)

        # Read bytecode array (uint64_t, little-endian)
        bytecode = []
        for _ in range(bytecode_size):
            instr, = struct.unpack("<Q", f.read(8))
            bytecode.append(instr)

    return {
        "version": version_number,
        "register_count": reg_count,
        "constants_size": constants_size,
        "bytecode_size": bytecode_size,
        "constants": constants,
        "bytecode": bytecode,
    }

class print_opcode:
    def start(registers,data, i):
        print()
        match data['bytecode'][i]:
            case OperationType.OP_LOAD_CONST.value:
                return print_opcode.OP_LOAD_CONST(registers,data, i)
            case OperationType.OP_DECLARE.value:
                return print_opcode.OP_DECLARE(registers,data, i)
            case OperationType.OP_LOAD_NULL.value:
                return print_opcode.OP_LOAD_NULL(registers,data, i)
                
    def OP_LOAD_CONST(registers,data, i) -> int:
        print("OP_LOAD_CONST ", end="")
        i+=1
        register = data["bytecode"][i]
        print("To Register",register,"", end="")
        i+=1
        match data["bytecode"][i]:
            case Types.TYPE_OP_STRING.value:
                print("TYPE_OP_STRING ", end="")
            case Types.TYPE_OP_NUMBER.value:
                print("TYPE_OP_NUMBER ", end="")
        i+=1
        length = data["bytecode"][i]
        i+=1
        offset = data["bytecode"][i]
        i+=1
        print("Length",length,"", end="")
        print("Offset",offset,"")
        registers[register] = data["constants"][offset:offset+length].decode()
        print("const value:", registers[register])
        return i

    def OP_DECLARE(registers,data, i) -> int:
        print("OP_DECLARE ", end="")
        i+=1
        length = data["bytecode"][i]
        i+=1
        offset = data["bytecode"][i]
        i+=1
        from_register = data["bytecode"][i]
        i+=1
        print("Name Length",length,"", end="")
        print("Name Offset",offset,"", end="")
        print("From Register",from_register,"")
        print("output: let", data['constants'][offset:offset+length].decode(),'=',registers[from_register])
        return i
    def OP_LOAD_NULL(registers,data, i) -> int:
        print("OP_LOAD_NULL ", end="")
        i+=1
        to_register = data["bytecode"][i]
        i+=1
        print("To Register",to_register,"")
        registers[to_register] = "null"
        return i
if __name__ == "__main__":
    filename = "out.arbin"
    data = read_arbin(filename)
    print(f"Version: {data['version']}")
    print(f"Register Count: {data['register_count']}")
    print(f"Constants Size: {data['constants_size']} bytes")
    print(f"Bytecode Length: {data['bytecode_size']} elements")

    registers = ["null"]*data['register_count']

    i=0
    while i<len(data["bytecode"]):
        i=print_opcode.start(registers,data,i)