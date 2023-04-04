import os

LIBRARY = "lv_pman.a"

Import("lv_pman_env")

sources = [lv_pman_env.Object(filename) for filename in Glob(
    os.path.join("src", "*.c"))]  # application files

lib = lv_pman_env.Library(LIBRARY, sources)

result = (lib, [os.getcwd()])
Return('result')
