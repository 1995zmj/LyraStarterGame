"""Unified entry: run in Unreal to register Tools menu and import all registered CSV groups."""
from pathlib import Path
import sys

script_dir = str(Path(__file__).resolve().parent)
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)
from arena_csv.manager import run, register_menu, open_location

if __name__ == "__main__":
    register_menu()
    run("import")
