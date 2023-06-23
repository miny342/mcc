with open("test/tmp.c", "r") as f:
    s = f.readlines()

with open("test/tmp.c", "w") as f:
    f.writelines(filter(lambda x: x[0] != "#", s))
