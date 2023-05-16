import os

LIBRARY = "lv_pman.a"

Import("pman_env")

sources = [pman_env.Object(filename) for filename in Glob(
    os.path.join("src", "*.c"))]  # application files

lib = pman_env.Library(LIBRARY, sources)

result = (lib, [os.getcwd()])
Return('result')
