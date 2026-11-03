"""Editor integration: CSV round trip, repeat import, and invalid-source atomicity."""
import csv
import hashlib
import runpy
import tempfile
from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[2]
module = runpy.run_path(str(root / "Content/Python/import_waves_csv.py"), run_name="wave_import_test")
validate = module["validate"]
import_config = module["import_config"]
waves, settings = validate()

for attempt in range(2):
    definition = import_config()
    entries = definition.get_editor_property("waves")
    assert len(entries) == len(waves)
    for entry, expected in zip(entries, waves):
        for prop, value in expected.items():
            assert abs(entry.get_editor_property(prop) - value) < 0.0001, prop
    for prop, value in settings.items():
        assert abs(definition.get_editor_property(prop) - value) < 0.0001, prop

bound = unreal.get_default_object(unreal.MonsterArenaComponent).get_editor_property("wave_definition")
assert bound.get_path_name() == definition.get_path_name()
asset_file = root / "Content/Data/DA_ArenaWaves.uasset"
before = hashlib.sha256(asset_file.read_bytes()).hexdigest()


def read(name):
    with (root / "Data" / name).open(encoding="utf-8-sig", newline="") as f:
        return list(csv.reader(f))


cases = [
    ("Waves.csv", 1, 0, "2"),
    ("Waves.csv", 2, 0, "1"),
    ("Waves.csv", 1, 1, "-1"),
    ("Waves.csv", 1, 1, "1.5"),
    ("Waves.csv", 1, 1, "10001"),
    ("Waves.csv", 1, 3, "0"),
    ("Waves.csv", 1, 3, "201"),
    ("Waves.csv", 1, 4, "7"),
    ("Waves.csv", 1, 5, "nan"),
    ("Waves.csv", 1, 5, "inf"),
    ("Waves.csv", 1, 5, "0"),
    ("Waves.csv", 1, 6, "5"),
    ("Waves.csv", 1, 2, ""),
    ("Waves.csv", 0, 0, "WrongHeader"),
    ("WaveSettings.csv", 1, 0, "-1"),
    ("WaveSettings.csv", 1, 1, "0"),
    ("WaveSettings.csv", 1, 2, "nan"),
]
for filename, row, col, value in cases:
    with tempfile.TemporaryDirectory(prefix="arena-wave-csv-") as directory:
        for name in ("Waves.csv", "WaveSettings.csv"):
            table = read(name)
            if name == filename:
                table[row][col] = value
            with (Path(directory) / name).open("w", encoding="utf-8-sig", newline="") as f:
                csv.writer(f).writerows(table)
        try:
            import_config(directory)
        except ValueError:
            pass
        else:
            raise AssertionError(f"Invalid input accepted: {filename}:{row}:{col}={value}")
        assert hashlib.sha256(asset_file.read_bytes()).hexdigest() == before, "Invalid CSV modified saved asset"

unreal.log("MONSTER_ARENA_WAVES_CSV_VALIDATION_SUCCEEDED: repeat import, all fields, binding, 17 invalid inputs")
