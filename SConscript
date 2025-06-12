import os

def rchop(s, suffix):
    if suffix and s.endswith(suffix):
        return s[:-len(suffix)]
    return s

LIBRARY = "lv_pman.a"

Import("pman_env")

sources = Glob("./src/*.c")

Import('pman_suffix')
if pman_suffix:
    objects = [pman_env.Object(
        f"{rchop(x.get_abspath(), '.c')}-{pman_suffix}", x) for x in sources]
else:
    objects = [pman_env.Object(x) for x in sources]

path = Dir('.').srcnode().abspath
result = (objects, list(map(lambda x: os.path.join(path, x), ["src"])))
Return("result")
