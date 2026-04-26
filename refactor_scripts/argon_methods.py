import re
import sys
from pathlib import Path

def convert_file(file_path, type_name):
    path = Path(file_path)
    text = path.read_text()

    # Match function definition:
    # ArgonObject *TYPE_method(...) { ... }
    pattern = re.compile(
        rf"""
        ArgonObject\s*\*\s*
        {re.escape(type_name)}_(?P<method>[A-Za-z0-9_]+)\s*
        \(
        (?P<args>[^)]*)
        \)\s*
        \{{                                # opening brace
        (?P<body>.*?)                      # function body (non-greedy)
        \n\}}
        """,
        re.VERBOSE | re.DOTALL
    )

    def replacer(match):
        method = match.group("method")
        body = match.group("body")

        # clean indentation (optional)
        body = body.rstrip()

        return f"ARGON_METHOD({type_name}, {method}, {{\n{body}\n}})"

    new_text = pattern.sub(replacer, text)
    path.write_text(new_text)

    print(f"Converted definitions for type: {type_name}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert.py <file> <TYPE_NAME>")
        sys.exit(1)

    convert_file(sys.argv[1], sys.argv[2])