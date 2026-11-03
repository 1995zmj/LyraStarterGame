"""Create the core-health widget once; preserve subsequent designer edits."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
blueprint = unreal.MonsterArenaCoreHealthWidgetLibrary.create_core_health_widget()
assert blueprint, "Could not create or load the core-health Widget Blueprint"
assert unreal.MonsterArenaCoreHealthWidgetLibrary.validate_core_health_widget(blueprint), "Core-health parent, bindings or compilation are invalid"
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
unreal.log("MONSTER_ARENA_CORE_HEALTH_SETUP_SUCCEEDED")
