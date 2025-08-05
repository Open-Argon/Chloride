def f():
    return g()
def g():
    return f

f()()