"""Create the HUD and its core-health child once; preserve designer edits."""
import runpy
from pathlib import Path
import unreal

runpy.run_path(str(Path(__file__).with_name("setup_monster_arena_core_health.py")), run_name="__main__")
unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
blueprint = unreal.MonsterArenaHUDLibrary.create_hud()
assert blueprint, "Could not create or load the HUD Widget Blueprint"
assert unreal.MonsterArenaHUDLibrary.validate_hud(blueprint), "HUD parent, bindings, slot or compilation are invalid"
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
unreal.log("MONSTER_ARENA_HUD_SETUP_SUCCEEDED")
