import re
import sys
from pathlib import Path

pattern = re.compile(
    r"""
    add_builtin_field\s*\(
        \s*(?P<type>[A-Za-z0-9_]+)\s*,\s*
        (?P<name>[A-Za-z0-9_]+)\s*,\s*
        create_argon_native_function\s*\(
            \s*"[^"]+"\s*,\s*
            (?P=type)_(?P=name)
        \s*\)
    \s*\)\s*;
    """,
    re.VERBOSE | re.DOTALL
)

def convert_file(file_path):
    path = Path(file_path)
    text = path.read_text()

    def replacer(match):
        type_name = match.group("type")
        name = match.group("name")
        return f"MOUNT_ARGON_METHOD({type_name}, {name})"

    new_text = pattern.sub(replacer, text)
    path.write_text(new_text)

    print(f"Converted: {file_path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python convert.py <file>")
        sys.exit(1)

    convert_file(sys.argv[1])