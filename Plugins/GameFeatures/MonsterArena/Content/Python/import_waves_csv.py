"""Compatibility entry: import only the registered waves configuration group."""
from pathlib import Path
import sys
script_dir = str(Path(__file__).resolve().parent)
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)
from arena_csv.waves import validate, DATA_DIR
from arena_csv.manager import run


def import_config(directory=DATA_DIR):
    validate(directory)
    run("import", ["waves"], directory)
    import unreal
    return unreal.load_asset("/MonsterArena/Data/DA_ArenaWaves")


if __name__ == "__main__":
    import_config()
