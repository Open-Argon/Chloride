# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

import string
from itertools import product
import sys

def generate_names(max_width, skip_keywords=None):
    if skip_keywords is None:
        skip_keywords = {"if", "else", "while", "forever", "for", "break", "continue",
                         "return", "let", "import", "from", "do", "true", "false", "null",
                         "delete", "not", "try", "catch", "in", "or", "and", "elif"}
    else:
        skip_keywords = set(skip_keywords)

    chars = string.ascii_lowercase
    first = True
    write = sys.stdout.write

    for length in range(1, max_width + 1):
        print(length, file=sys.stderr)
        i = 0
        for p in product(chars, repeat=length):
            name = ''.join(p)
            if name in skip_keywords:
                continue
            write('let ')
            write(name)
            write(' = null\n')
            first = False
            if i>10000000:
                break
            i+=1

# Example usage:
max_width = 5
generate_names(max_width)
