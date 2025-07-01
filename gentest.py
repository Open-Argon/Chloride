import random
myfile = open("rand_test.ar","w")

for i in range(10000000):
  myfile.write("\"")
  myfile.write(str(random.random())[2::])
  myfile.write("\"\n")
