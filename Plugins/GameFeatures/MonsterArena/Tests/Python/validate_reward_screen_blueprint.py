"""Validate the saved reward screen layout and its HUD binding."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
for name, validator in (("W_MonsterArenaRewardStat", unreal.MonsterArenaRewardCardLibrary.validate_stat),
                        ("W_MonsterArenaRewardCard", unreal.MonsterArenaRewardCardLibrary.validate_card)):
    view = unreal.load_asset("/MonsterArena/UI/" + name)
    assert view and validator(view)
blueprint = unreal.load_asset("/MonsterArena/UI/W_MonsterArenaRewardScreen")
assert blueprint and unreal.MonsterArenaRewardScreenLibrary.validate_reward_screen(blueprint)
screen_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaRewardScreen.W_MonsterArenaRewardScreen_C")
assert unreal.MathLibrary.class_is_child_of(screen_class, unreal.MonsterArenaRewardScreen)
hud_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD_C")
assert unreal.get_default_object(hud_class).get_editor_property("reward_screen_class") == screen_class
unreal.log("MONSTER_ARENA_REWARD_SCREEN_VALIDATION_SUCCEEDED")
