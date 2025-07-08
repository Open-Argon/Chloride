import random
import sys

for i in range(100000000):
  sys.stdout.write("\"")
  sys.stdout.write(str(random.random()))
  sys.stdout.write("\"\n")
