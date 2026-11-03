"""Authoritative wave CSV -> runtime DataAsset. No asset mutation before full validation."""
from .common import read_rows
import math
from pathlib import Path

DATA_DIR = Path(__file__).resolve().parents[3] / "Data"
WAVE_COLUMNS = "Wave AssaultCount HunterCount AliveLimit BatchSize BatchInterval EntranceCount".split()
SETTINGS_COLUMNS = "WarningDuration WithinBatchInterval PlayerSafeDistance".split()
ASSET_PATH = "/MonsterArena/Data/DA_ArenaWaves"
SOURCES = {"Waves.csv", "WaveSettings.csv"}


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
            "assault_count": number(row, "AssaultCount", location, 0, 10000),
            "hunter_count": number(row, "HunterCount", location, 0, 10000),
            "alive_limit": number(row, "AliveLimit", location, 1, 200),
            "batch_size": number(row, "BatchSize", location, 1, 200),
            "batch_interval": number(row, "BatchInterval", location, 0.3, 3600, False),
            "entrance_count": number(row, "EntranceCount", location, 1, 4),
        }
        if not 1 <= values["assault_count"] + values["hunter_count"] <= 10000:
            raise ValueError(f"{location}: total enemy count must be 1..10000")
        if values["batch_size"] > values["alive_limit"]:
            raise ValueError(f"{location}: BatchSize cannot exceed AliveLimit")
        waves.append(values)
    settings_rows = read_rows(directory, "WaveSettings.csv", SETTINGS_COLUMNS)
    if len(settings_rows) != 1:
        raise ValueError("WaveSettings.csv: exactly one settings row is required")
    row = settings_rows[0]
    settings = {
        "warning_duration": number(row, "WarningDuration", "WaveSettings.csv:2", 0, 120, False),
        "within_batch_interval": number(row, "WithinBatchInterval", "WaveSettings.csv:2", 0.1, 60, False),
        "player_safe_distance": number(row, "PlayerSafeDistance", "WaveSettings.csv:2", 0, 10000, False),
    }
    return waves, settings


def apply(snapshot):
    waves, settings = snapshot
    import unreal
    unreal.load_module("MonsterArenaRuntime")
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
    entries = []
    for values in waves:
        entry = unreal.ArenaWaveConfig()
        for prop, value in values.items():
            entry.set_editor_property(prop, value)
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
    definition.set_editor_property("waves", entries)
    for prop, value in settings.items():
        definition.set_editor_property(prop, value)
    if not assets.save_loaded_asset(definition, only_if_is_dirty=False):
        raise RuntimeError("Could not save wave configuration asset")
    unreal.log("MONSTER_ARENA_WAVES_CSV_IMPORT_SUCCEEDED")
    return definition



