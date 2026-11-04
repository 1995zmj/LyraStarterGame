"""Validate the saved MonsterArena input chain."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)

open_action = unreal.load_asset("/MonsterArena/Input/Actions/IA_MonsterArena_OpenRewards")
wave_action = unreal.load_asset("/MonsterArena/Input/Actions/IA_MonsterArena_WaveAction")
context = unreal.load_asset("/MonsterArena/Input/Mappings/IMC_MonsterArena")
assert open_action, "IA_MonsterArena_OpenRewards is missing"
assert wave_action, "IA_MonsterArena_WaveAction is missing"
assert context, "IMC_MonsterArena is missing"
assert unreal.MathLibrary.class_is_child_of(open_action.get_class(), unreal.MonsterArenaRewardInputAction), \
    "IA_MonsterArena_OpenRewards must use UMonsterArenaRewardInputAction"
assert open_action.get_editor_property("value_type") == unreal.InputActionValueType.BOOLEAN
assert wave_action.get_editor_property("value_type") == unreal.InputActionValueType.BOOLEAN
assert unreal.load_asset("/MonsterArena/Input/InputData_MonsterArena"), "InputData_MonsterArena is missing"
assert unreal.load_asset("/MonsterArena/System/AbilitySet_MonsterArena"), "AbilitySet_MonsterArena is missing"

hud_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD_C")
assert hud_class and unreal.MathLibrary.class_is_child_of(hud_class, unreal.MonsterArenaHUD)
assert unreal.MonsterArenaInputLibrary.validate_input_assets(), \
    "Saved input assets do not contain the expected input tag, ability grant, action set or experience wiring"
unreal.log("MONSTER_ARENA_INPUT_VALIDATION_SUCCEEDED")
