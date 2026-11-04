"""Unified import regression. Uses temporary CSV sources and real Unreal assets."""
import csv
import json
import runpy
import shutil
import sys
import tempfile
from pathlib import Path
from unittest.mock import patch
import unreal

ROOT = Path(__file__).resolve().parents[2]
SCRIPT_DIR = ROOT / "Content/Python"
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
from arena_csv import manager

checks = []


def check(value, label):
    assert value, label
    checks.append(label)
    unreal.log("MonsterArenaCSVManager: PASS: " + label)


listing = manager.run("list")
check(len(listing["groups"]) == 2, "Registry lists both configuration groups")
check(sum(len(g["sources"]) for g in listing["groups"]) == 7, "Registry owns all seven CSV sources")
check(manager.run("validate")["success"], "All source groups validate without writes")
groups = manager.registry()
check(manager.ordered_groups(groups, ["waves"]) == ["waves"], "Single-group selection stays scoped")
linked = {key: dict(value) for key, value in groups.items()}
linked["waves"]["depends_on"] = ["progression"]
check(manager.ordered_groups(linked, ["waves"]) == ["progression", "waves"], "Dependencies import first")
linked["progression"]["depends_on"] = ["waves"]
try:
    manager.ordered_groups(linked)
except ValueError:
    check(True, "Dependency cycle rejected")
else:
    raise AssertionError("Dependency cycle accepted")

with tempfile.TemporaryDirectory(prefix="arena-config-manager-") as temporary:
    target = Path(temporary)
    for source in (ROOT / "Data").iterdir():
        if source.suffix in (".csv", ".json"):
            shutil.copy2(source, target / source.name)
    wave_path = target / "Waves.csv"
    original = wave_path.read_text(encoding="utf-8-sig")
    wave_path.write_text(original.replace("1,6,2,4,1", "1,0,2,4,1"), encoding="utf-8")
    with patch.object(manager.progression, "apply") as write_progression, patch.object(manager.waves, "apply") as write_waves:
        try:
            manager.run("import", directory=target)
        except ValueError as error:
            check("waves" in str(error) and "AliveLimit" in str(error), "Validation failure identifies group and field")
        else:
            raise AssertionError("Bad wave accepted")
        check(not write_progression.called and not write_waves.called, "Bad later group prevents every asset writer")
    wave_path.write_text(original, encoding="utf-8")
    (target / "Forgotten.csv").write_text("Id,Value\na,1\n", encoding="utf-8")
    try:
        manager.prepare(directory=target)
    except ValueError as error:
        check("Forgotten.csv" in str(error), "Unregistered CSV cannot silently escape management")
    else:
        raise AssertionError("Unregistered CSV accepted")
    (target / "Forgotten.csv").unlink()
    original_validate = manager.progression.validate

    def change_during_validation(directory):
        snapshot = original_validate(directory)
        wave_path.write_text(original.replace("1,6,2,4,1", "1,6,2,4.5,1"), encoding="utf-8")
        return snapshot

    with patch.object(manager.progression, "validate", side_effect=change_during_validation):
        try:
            manager.prepare(directory=target)
        except ValueError as error:
            check("changed during validation" in str(error), "Concurrent source edits invalidate the import snapshot")
        else:
            raise AssertionError("Source edit not detected")

# Use real adapters and real editor asset saves, twice.
with patch.object(manager.progression, "apply"), patch.object(manager.waves, "apply", side_effect=RuntimeError("Simulated asset save failure")):
    try:
        manager.run("import")
    except RuntimeError:
        reports = sorted((ROOT.parents[2] / "Saved/ConfigImports/MonsterArena").glob("*.json"))
        failed = json.loads(reports[-1].read_text(encoding="utf-8"))
        check(not failed["success"] and [e["status"] for e in failed["groups"]] == ["imported", "failed"], "Partial writer failure is reported honestly")
    else:
        raise AssertionError("Writer failure accepted")

for attempt in range(2):
    report = manager.run("import")
    check(report["success"] and all(e["status"] == "imported" for e in report["groups"]), "Unified real import " + str(attempt + 1))
    saved = json.loads(Path(report["report_path"]).read_text(encoding="utf-8"))
    check(saved["success"] and len(saved["source_sha256"]) == 8, "Report persists status and all source fingerprints")

# Existing entry points must still validate and author the same assets.
runpy.run_path(str(ROOT / "Tests/Python/validate_csv_configuration.py"), run_name="__main__")
check(True, "Progression compatibility entry and original invalid-input regression")
runpy.run_path(str(ROOT / "Tests/Python/validate_wave_csv.py"), run_name="__main__")
check(True, "Waves compatibility entry and original invalid-input regression")
unreal.log("MONSTER_ARENA_CSV_MANAGER_SUCCEEDED: " + str(len(checks)) + " checks")
