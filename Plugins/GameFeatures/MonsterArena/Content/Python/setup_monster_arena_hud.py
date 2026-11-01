"""Create the HUD once; preserve subsequent designer edits."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
blueprint = unreal.MonsterArenaHUDLibrary.create_hud()
assert blueprint, "Could not create or load the HUD Widget Blueprint"
assert unreal.MonsterArenaHUDLibrary.validate_hud(blueprint), "HUD parent, bindings or compilation are invalid"
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
unreal.log("MONSTER_ARENA_HUD_SETUP_SUCCEEDED")
