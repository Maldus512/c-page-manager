from pathlib import Path
import os

LIBRARY = "lv_pman.a"

Import("lv_pman_env")

sources = []
sources = [File(filename) for filename in Path(
    "src").rglob("*.c")]  # application files

lib = lv_pman_env.Library(LIBRARY, sources)

result = (lib, [os.getcwd()])
Return('result')
