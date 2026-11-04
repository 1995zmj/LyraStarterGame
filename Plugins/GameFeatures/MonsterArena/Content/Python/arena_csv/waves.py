"""Authoritative wave/monster CSV -> runtime DataAsset. No asset mutation before full validation."""
from .common import read_rows
import math
import re
from pathlib import Path

DATA_DIR = Path(__file__).resolve().parents[3] / "Data"
WAVE_COLUMNS = "Wave AliveLimit BatchSize BatchInterval EntranceCount".split()
MONSTER_COLUMNS = "MonsterId InitialHealth MeleeDamage MoveSpeed ExperienceReward DetectionRadius MeleeRange AttackInterval BodyScale".split()
WAVE_MONSTER_COLUMNS = "Wave MonsterId Count Role".split()
SETTINGS_COLUMNS = "WarningDuration WithinBatchInterval PlayerSafeDistance".split()
ASSET_PATH = "/MonsterArena/Data/DA_ArenaWaves"
SOURCES = {"Waves.csv", "WaveMonsters.csv", "Monsters.csv", "WaveSettings.csv"}
ROLES = {"assault": False, "hunter": True}
MONSTER_ID = re.compile(r"[A-Za-z][A-Za-z0-9_]{0,63}")


def number(row, key, location, minimum, maximum, integer=True):
    try:
        value = int(row[key]) if integer else float(row[key])
        if not math.isfinite(value) or not minimum <= value <= maximum:
            raise ValueError()
        return value
    except (ValueError, OverflowError):
        raise ValueError(f"{location}: invalid {key}={row[key]!r}; expected {minimum}..{maximum}") from None


def validate(directory=DATA_DIR):
    source = read_rows(directory, "Waves.csv", WAVE_COLUMNS)
    if len(source) > 1000:
        raise ValueError("Waves.csv: at most 1000 waves")
    waves = []
    for index, row in enumerate(source, 1):
        location = f"Waves.csv:{index + 1}"
        if number(row, "Wave", location, 1, 1000) != index:
            raise ValueError(f"{location}: Wave must be consecutive starting at 1")
        values = {
            "alive_limit": number(row, "AliveLimit", location, 1, 200),
            "batch_size": number(row, "BatchSize", location, 1, 200),
            "batch_interval": number(row, "BatchInterval", location, 0.3, 3600, False),
            "entrance_count": number(row, "EntranceCount", location, 1, 4),
        }
        if values["batch_size"] > values["alive_limit"]:
            raise ValueError(f"{location}: BatchSize cannot exceed AliveLimit")
        waves.append(values)

    monster_rows = read_rows(directory, "Monsters.csv", MONSTER_COLUMNS)
    if len(monster_rows) > 512:
        raise ValueError("Monsters.csv: at most 512 monster types")
    monsters, monster_ids = [], {}
    for index, row in enumerate(monster_rows, 1):
        location = f"Monsters.csv:{index + 1}"
        identity = row["MonsterId"].strip()
        if not MONSTER_ID.fullmatch(identity):
            raise ValueError(f"{location}: MonsterId must be an English letter followed by letters, digits or underscores")
        key = identity.casefold()
        if key in monster_ids:
            raise ValueError(f"{location}: duplicate MonsterId={identity!r}")
        monster_ids[key] = identity
        monsters.append({
            "monster_id": identity,
            "initial_health": number(row, "InitialHealth", location, 1, 100000, False),
            "melee_damage": number(row, "MeleeDamage", location, 0, 100000, False),
            "move_speed": number(row, "MoveSpeed", location, 1, 2000, False),
            "experience_reward": number(row, "ExperienceReward", location, 1, 100000),
            "detection_radius": number(row, "DetectionRadius", location, 1, 100000, False),
            "melee_range": number(row, "MeleeRange", location, 1, 100000, False),
            "attack_interval": number(row, "AttackInterval", location, 0.1, 100, False),
            "body_scale": number(row, "BodyScale", location, 0.1, 10, False),
        })

    selection_rows = read_rows(directory, "WaveMonsters.csv", WAVE_MONSTER_COLUMNS)
    selections = [[] for _ in waves]
    used = set()
    for index, row in enumerate(selection_rows, 1):
        location = f"WaveMonsters.csv:{index + 1}"
        wave = number(row, "Wave", location, 1, len(waves))
        key = row["MonsterId"].strip().casefold()
        if key not in monster_ids:
            raise ValueError(f"{location}: unknown MonsterId={row['MonsterId']!r}; add it to Monsters.csv first")
        count = number(row, "Count", location, 1, 10000)
        role = row["Role"].strip().casefold()
        if role not in ROLES:
            raise ValueError(f"{location}: Role must be Assault or Hunter")
        if (wave, key) in used:
            raise ValueError(f"{location}: wave {wave} lists MonsterId={monster_ids[key]!r} more than once")
        used.add((wave, key))
        selections[wave - 1].append({"monster_id": monster_ids[key], "count": count, "hunter": ROLES[role]})
    for wave, entries in enumerate(selections, 1):
        if not entries:
            raise ValueError(f"WaveMonsters.csv: wave {wave} has no monsters")
        if sum(entry["count"] for entry in entries) > 10000:
            raise ValueError(f"WaveMonsters.csv: wave {wave} total enemy count must be at most 10000")

    settings_rows = read_rows(directory, "WaveSettings.csv", SETTINGS_COLUMNS)
    if len(settings_rows) != 1:
        raise ValueError("WaveSettings.csv: exactly one settings row is required")
    row = settings_rows[0]
    settings = {
        "warning_duration": number(row, "WarningDuration", "WaveSettings.csv:2", 0, 120, False),
        "within_batch_interval": number(row, "WithinBatchInterval", "WaveSettings.csv:2", 0.1, 60, False),
        "player_safe_distance": number(row, "PlayerSafeDistance", "WaveSettings.csv:2", 0, 10000, False),
    }
    return waves, monsters, selections, settings


def apply(snapshot):
    waves, monsters, selections, settings = snapshot
    import unreal
    unreal.load_module("MonsterArenaRuntime")
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
    catalog = []
    for values in monsters:
        config = unreal.ArenaMonsterConfig()
        for prop, value in values.items():
            config.set_editor_property(prop, value)
        catalog.append(config)
    entries = []
    for values, selected in zip(waves, selections):
        entry = unreal.ArenaWaveConfig()
        for prop, value in values.items():
            entry.set_editor_property(prop, value)
        choices = []
        for choice in selected:
            monster = unreal.ArenaWaveMonsterEntry()
            monster.set_editor_property("monster_id", choice["monster_id"])
            monster.set_editor_property("count", choice["count"])
            monster.set_editor_property("hunter", choice["hunter"])
            choices.append(monster)
        entry.set_editor_property("monsters", choices)
        entries.append(entry)
    assets = unreal.EditorAssetLibrary
    definition = unreal.load_asset(ASSET_PATH) if assets.does_asset_exist(ASSET_PATH) else None
    if definition is not None and not isinstance(definition, unreal.ArenaWaveDefinition):
        raise TypeError(f"{ASSET_PATH}: expected ArenaWaveDefinition")
    if definition is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.ArenaWaveDefinition)
        definition = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_ArenaWaves", "/MonsterArena/Data", unreal.ArenaWaveDefinition, factory)
    if definition is None:
        raise RuntimeError("Could not create wave configuration asset")
    definition.set_editor_property("monsters", catalog)
    definition.set_editor_property("waves", entries)
    for prop, value in settings.items():
        definition.set_editor_property(prop, value)
    if not assets.save_loaded_asset(definition, only_if_is_dirty=False):
        raise RuntimeError("Could not save wave configuration asset")
    unreal.log("MONSTER_ARENA_WAVES_CSV_IMPORT_SUCCEEDED")
    return definition
