def test(*d, **e):
  print(d, e)

x = ["bruh"]
e={"bruh":"moment"}

test(*x, **e)