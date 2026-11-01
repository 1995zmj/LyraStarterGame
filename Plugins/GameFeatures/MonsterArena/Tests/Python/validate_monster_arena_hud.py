"""Check the saved HUD designer tree and native binding contract."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
path = "/MonsterArena/UI/W_MonsterArenaHUD"
blueprint = unreal.load_asset(path)
assert blueprint
assert unreal.MonsterArenaHUDLibrary.validate_hud(blueprint)
cls = unreal.load_class(None, path + ".W_MonsterArenaHUD_C")
assert unreal.MathLibrary.class_is_child_of(cls, unreal.MonsterArenaHUD)
unreal.log("MONSTER_ARENA_HUD_VALIDATION_SUCCEEDED")
