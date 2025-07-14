# SPDX-FileCopyrightText: 2025 William Bell
#
# SPDX-License-Identifier: GPL-3.0-or-later

import random
import sys

for i in range(10000):
  sys.stdout.write("\"")
  sys.stdout.write(str(random.random()))
  sys.stdout.write("\"\n")
