"""Repeat the import and check saved configuration references and invalid input."""
import csv
import pathlib
import runpy
import tempfile
import unreal

root = pathlib.Path(__file__).resolve().parents[2]
script = root / "Content/Python/import_progression_csv.py"
runpy.run_path(str(script), run_name="__main__")
definition = unreal.load_asset("/MonsterArena/Data/DA_MonsterArenaProgression")
with (root / "Data/Levels.csv").open(encoding="utf-8-sig", newline="") as stream:
    expected = [int(row["ExperienceToNextLevel"]) for row in csv.DictReader(stream)]
assert list(definition.get_editor_property("experience_per_level")) == expected
pool = definition.get_editor_property("reward_pool")
assert len(pool.get_editor_property("cards")) == 6
cls = unreal.load_class(None, "/MonsterArena/Data/B_MonsterArenaProgression.B_MonsterArenaProgression_C")
assert unreal.get_default_object(cls).get_editor_property("definition") == definition
# Execute only the validation phase against temporary sources. Any malformed
# source must fail before the first Unreal module/asset mutation operation.
prefix = script.read_text(encoding="utf-8-sig").split('unreal.load_module("MonsterArenaRuntime")')[0]
with tempfile.TemporaryDirectory() as temporary:
    target = pathlib.Path(temporary)
    (target / "Data").mkdir()
    fake_script = target / "Content/Python/import_progression_csv.py"
    originals = {name: (root / "Data" / name).read_text(encoding="utf-8-sig") for name in ("Levels.csv", "RewardCards.csv", "RewardActions.csv")}
    cases = [
        ("Levels.csv", originals["Levels.csv"].replace("1,60", "2,60")),
        ("Levels.csv", originals["Levels.csv"].replace("1,60", "1,-5")),
        ("RewardCards.csv", originals["RewardCards.csv"].replace("Vitality,", "Firepower,")),
        ("RewardActions.csv", originals["RewardActions.csv"].replace("HealFraction,0.4", "HealFraction,nan")),
        ("RewardActions.csv", originals["RewardActions.csv"].replace("DamageBonus,3", "UnknownBehavior,3")),
        ("RewardActions.csv", originals["RewardActions.csv"].replace("Firepower,", "MissingCard,")),
        ("RewardActions.csv", originals["RewardActions.csv"].replace("SpareAmmo,60", "SpareAmmo,0.5")),
        ("RewardActions.csv", originals["RewardActions.csv"].replace("DamageBonus,3", "DamageBonus,-1")),
        ("RewardActions.csv", originals["RewardActions.csv"].replace("Firepower,DamageBonus,3\n", "")),
        ("RewardActions.csv", originals["RewardActions.csv"] + "FieldMedicine,HealFraction,0.8\n"),
        ("RewardActions.csv", originals["RewardActions.csv"] + "AmmoSupply,SpareAmmo,16777216\n"),
    ]
    for filename, bad in cases:
        for name, original in originals.items():
            (target / "Data" / name).write_text(bad if name == filename else original, encoding="utf-8")
        try:
            exec(compile(prefix, str(script), "exec"), {"__file__": str(fake_script)})
        except ValueError:
            pass
        else:
            raise AssertionError(f"Invalid data was accepted: {filename} {bad}")
    # Multiple rows of the same behavior are legal, and preserve composition.
    for name, original in originals.items():
        (target / "Data" / name).write_text(original, encoding="utf-8")
    with (target / "Data/RewardActions.csv").open("a", encoding="utf-8") as stream:
        stream.write("Balance,DamageBonus,2\n")
    namespace = {"__file__": str(fake_script)}
    exec(compile(prefix, str(script), "exec"), namespace)
    assert namespace["actions"]["balance"] == [("MaxHealthBonus", 10), ("DamageBonus", 1), ("HealAmount", 10), ("DamageBonus", 2)]
# Check every imported action, not just the number of cards.
expected_actions = {}
with (root / "Data/RewardActions.csv").open(encoding="utf-8-sig", newline="") as stream:
    for row in csv.DictReader(stream):
        expected_actions.setdefault(row["CardId"], []).append((row["Behavior"], float(row["Value"])))
enums = {"MaxHealthBonus": unreal.RunRewardBehavior.MAX_HEALTH_BONUS,
         "DamageBonus": unreal.RunRewardBehavior.DAMAGE_BONUS,
         "HealAmount": unreal.RunRewardBehavior.HEAL_AMOUNT,
         "HealFraction": unreal.RunRewardBehavior.HEAL_FRACTION,
         "SpareAmmo": unreal.RunRewardBehavior.SPARE_AMMO, "DamagePerKill": unreal.RunRewardBehavior.DAMAGE_PER_KILL}
for entry in pool.cards:
    expected = expected_actions[str(entry.option.card_id)]
    assert len(entry.option.actions) == len(expected)
    for action, (kind, value) in zip(entry.option.actions, expected):
        assert action.behavior == enums[kind] and abs(action.value - value) < 0.00001
unreal.log("MONSTER_ARENA_CSV_VALIDATION_SUCCEEDED")
