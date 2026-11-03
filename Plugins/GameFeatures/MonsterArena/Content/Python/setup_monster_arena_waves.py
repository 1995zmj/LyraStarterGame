"""Idempotent wave configuration and defense map authoring; preserves existing edits."""
import unreal
import runpy
from pathlib import Path

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/MonsterArena/Maps/L_MonsterArena")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert unreal.ArenaWaveContentLibrary.setup_defense_map(world)
runpy.run_path(str(Path(__file__).with_name("import_waves_csv.py")), run_name="__main__")
assert levels.save_current_level()
unreal.log("MONSTER_ARENA_WAVES_SETUP_SUCCEEDED")
