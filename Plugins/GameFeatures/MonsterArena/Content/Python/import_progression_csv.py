"""Editor-only CSV import. Validate all source rows before changing assets."""
import csv
import math
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]

def rows(name, columns):
    with (ROOT / "Data" / name).open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != columns:
            raise ValueError(f"{name}: expected columns {columns}")
        result = list(reader)
        if not result:
            raise ValueError(f"{name}: table is empty")
        for index, row in enumerate(result, 2):
            if None in row or any(value is None or not value.strip() for value in row.values()):
                raise ValueError(f"{name}:{index}: missing or extra field")
        return result

def number(row, key, location, integer=False, minimum=0, maximum=2147483647):
    try:
        value = int(row[key]) if integer else float(row[key])
        if not math.isfinite(value) or not minimum <= value <= maximum:
            raise ValueError()
        return value
    except ValueError:
        raise ValueError(f"{location}: invalid {key}={row[key]!r}") from None

levels = rows("Levels.csv", ["Level", "ExperienceToNextLevel"])
experience = []
for index, row in enumerate(levels, 1):
    location = f"Levels.csv:{index + 1}"
    if number(row, "Level", location, True, 1) != index:
        raise ValueError(f"{location}: levels must be consecutive starting at 1")
    experience.append(number(row, "ExperienceToNextLevel", location, True, 1))

columns = "CardId Title Description Weight MinimumLevel MaxStacks".split()
cards = rows("RewardCards.csv", columns)
seen = set()
validated = []
for index, row in enumerate(cards, 2):
    location = f"RewardCards.csv:{index}"
    identity = row["CardId"].strip()
    if identity.lower() in seen or identity.lower() == "none" or not identity.isascii() or not identity.replace("_", "").isalnum():
        raise ValueError(f"{location}: invalid or duplicate CardId")
    seen.add(identity.lower())
    values = {key: number(row, key, location, key in ("MinimumLevel", "MaxStacks"), 2 if key == "MinimumLevel" else 0) for key in columns[3:]}
    validated.append((identity, row, values))

# A card owns any number of behavior/value records, without effect-specific columns.
behaviors = ("MaxHealthBonus", "DamageBonus", "HealAmount", "HealFraction", "SpareAmmo", "DamagePerKill")
actions = {identity.lower(): [] for identity, _, _ in validated}
for index, row in enumerate(rows("RewardActions.csv", ["CardId", "Behavior", "Value"]), 2):
    location = f"RewardActions.csv:{index}"
    identity = row["CardId"].strip().lower()
    behavior = row["Behavior"].strip()
    if identity not in actions:
        raise ValueError(f"{location}: unknown CardId")
    if behavior not in behaviors:
        raise ValueError(f"{location}: unknown Behavior={behavior!r}")
    value = number(row, "Value", location, behavior == "SpareAmmo", 0, 1 if behavior == "HealFraction" else 16777216)
    if value <= 0:
        raise ValueError(f"{location}: Value must be positive; remove unused actions")
    actions[identity].append((behavior, value))
for identity, entries in actions.items():
    if not entries:
        raise ValueError(f"RewardActions.csv: {identity} has no actions")
    for behavior in behaviors:
        total = sum(value for kind, value in entries if kind == behavior)
        if total > (1 if behavior == "HealFraction" else 16777216):
            raise ValueError(f"RewardActions.csv: {identity} exceeds total limit for {behavior}")
if not any(v["Weight"] > 0 and v["MinimumLevel"] == 2 and any(kind in ("DamageBonus", "MaxHealthBonus", "DamagePerKill") for kind, _ in actions[identity.lower()]) for identity, _, v in validated):
    raise ValueError("RewardCards.csv: provide a permanent reward eligible at level 2")

unreal.load_module("MonsterArenaRuntime")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
def asset(name, cls):
    path = "/MonsterArena/Data/" + name
    existing = unreal.load_asset(path) if assets.does_asset_exist(path) else None
    if existing:
        if not isinstance(existing, cls):
            raise TypeError(path)
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, "/MonsterArena/Data", cls, factory)

pool = asset("DA_MonsterArenaRewardPool", unreal.RunRewardPoolDefinition)
definition = asset("DA_MonsterArenaProgression", unreal.RunProgressionDefinition)
entries = []
mapping = dict(MaxHealthBonus=unreal.RunRewardBehavior.MAX_HEALTH_BONUS,
               DamageBonus=unreal.RunRewardBehavior.DAMAGE_BONUS,
               HealAmount=unreal.RunRewardBehavior.HEAL_AMOUNT,
               HealFraction=unreal.RunRewardBehavior.HEAL_FRACTION,
               SpareAmmo=unreal.RunRewardBehavior.SPARE_AMMO, DamagePerKill=unreal.RunRewardBehavior.DAMAGE_PER_KILL)
for identity, row, values in validated:
    option = unreal.RunRewardOption()
    option.set_editor_property("card_id", identity)
    option.set_editor_property("title", row["Title"])
    option.set_editor_property("description", row["Description"])
    reward_actions = []
    for behavior, value in actions[identity.lower()]:
        action = unreal.RunRewardAction()
        action.set_editor_property("behavior", mapping[behavior])
        action.set_editor_property("value", value)
        reward_actions.append(action)
    option.set_editor_property("actions", reward_actions)
    entry = unreal.RunRewardCardEntry()
    entry.set_editor_property("option", option)
    for column, prop in (("Weight", "weight"), ("MinimumLevel", "minimum_level"), ("MaxStacks", "max_stacks")):
        entry.set_editor_property(prop, values[column])
    entries.append(entry)
pool.set_editor_property("cards", entries)
definition.set_editor_property("experience_per_level", experience)
definition.set_editor_property("reward_pool", pool)
path = "/MonsterArena/Data/B_MonsterArenaProgression"
blueprint = unreal.load_asset(path) if assets.does_asset_exist(path) else None
if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.RunProgressionComponent)
    blueprint = tools.create_asset("B_MonsterArenaProgression", "/MonsterArena/Data", unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
component_class = unreal.load_class(None, path + ".B_MonsterArenaProgression_C")
unreal.get_default_object(component_class).set_editor_property("definition", definition)
experience_class = unreal.load_class(None, "/MonsterArena/System/Experiences/B_MonsterArena.B_MonsterArena_C")
experience_default = unreal.get_default_object(experience_class)
unreal.load_module("MonsterArenaEditor")
assert unreal.MonsterArenaContentLibrary.bind_progression(experience_default, component_class)
for obj in (pool, definition, blueprint, unreal.load_asset("/MonsterArena/System/Experiences/B_MonsterArena")):
    assert assets.save_loaded_asset(obj, only_if_is_dirty=False)
unreal.log("MONSTER_ARENA_CSV_IMPORT_SUCCEEDED")
