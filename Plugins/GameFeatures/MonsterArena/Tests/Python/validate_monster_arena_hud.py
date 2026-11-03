"""Check the saved HUD designer tree, the core-health slot and native bindings."""
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
core_path = "/MonsterArena/UI/W_MonsterArenaCoreHealth"
core = unreal.load_asset(core_path)
assert core
assert unreal.MonsterArenaCoreHealthWidgetLibrary.validate_core_health_widget(core)
core_cls = unreal.load_class(None, core_path + ".W_MonsterArenaCoreHealth_C")
assert unreal.MathLibrary.class_is_child_of(core_cls, unreal.MonsterArenaCoreHealthWidget)
unreal.log("MONSTER_ARENA_HUD_VALIDATION_SUCCEEDED")
