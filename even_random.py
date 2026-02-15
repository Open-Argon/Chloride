import random

x=0
i=0
while (i<1e6):
  x=x+random.random()
  i=i+1
  if (i%1e5 == 0): print(i,x,x/i)