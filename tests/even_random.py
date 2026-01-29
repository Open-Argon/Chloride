import random

x=0
i=0
while (True):
  x=x+random.random()
  i=i+1
  if (i%1e6 == 0): print(i,x/i)