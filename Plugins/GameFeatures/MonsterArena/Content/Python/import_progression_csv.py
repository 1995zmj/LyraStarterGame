"""Compatibility entry: import only the registered progression configuration group."""
from pathlib import Path
import sys
script_dir = str(Path(__file__).resolve().parent)
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)
from arena_csv.progression import validate
from arena_csv.manager import run

if __name__ == "__main__":
    run("import", ["progression"])
