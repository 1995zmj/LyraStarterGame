"""Create the MonsterArena input chain once; preserve designer edits.

The chain mirrors IA_ShowScoreboard: IMC -> InputAction -> LyraInputConfig
(InputTag) -> AbilitySet -> GameplayAbility. The ability opens the existing
CommonUI reward screen, so server validation and input blocking stay in C++.
"""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)

context = unreal.MonsterArenaInputLibrary.create_input_assets()
assert context, "Could not create or load the MonsterArena input setup"
assert unreal.MonsterArenaInputLibrary.validate_input_assets(), "MonsterArena input setup is invalid"
for path in [
    "/MonsterArena/Input/Actions/IA_MonsterArena_OpenRewards",
    "/MonsterArena/Input/Actions/IA_MonsterArena_WaveAction",
    "/MonsterArena/Input/Mappings/IMC_MonsterArena",
    "/MonsterArena/Input/InputData_MonsterArena",
    "/MonsterArena/System/AbilitySet_MonsterArena",
    "/MonsterArena/System/Experiences/LAS_MonsterArena_Components",
]:
    asset = unreal.load_asset(path)
    assert asset, path
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), path

experience = unreal.load_asset("/MonsterArena/System/Experiences/B_MonsterArena")
assert experience, "B_MonsterArena is missing"
unreal.BlueprintEditorLibrary.compile_blueprint(experience)
assert unreal.EditorAssetLibrary.save_loaded_asset(experience, only_if_is_dirty=False)
unreal.log("MONSTER_ARENA_INPUT_SETUP_SUCCEEDED")
