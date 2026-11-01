"""Create the reward screen once and bind the saved HUD to its Blueprint class."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
for creator, validator in ((unreal.MonsterArenaRewardCardLibrary.create_stat, unreal.MonsterArenaRewardCardLibrary.validate_stat),
                           (unreal.MonsterArenaRewardCardLibrary.create_card, unreal.MonsterArenaRewardCardLibrary.validate_card)):
    child = creator()
    assert child and validator(child)
    assert unreal.EditorAssetLibrary.save_loaded_asset(child, only_if_is_dirty=False)
stat_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaRewardStat.W_MonsterArenaRewardStat_C")
card_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaRewardCard.W_MonsterArenaRewardCard_C")
unreal.get_default_object(card_class).set_editor_property("effect_row_class", stat_class)
assert unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset("/MonsterArena/UI/W_MonsterArenaRewardCard"), only_if_is_dirty=False)
blueprint = unreal.MonsterArenaRewardScreenLibrary.create_reward_screen()
assert blueprint
assert unreal.MonsterArenaRewardScreenLibrary.validate_reward_screen(blueprint)
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
screen_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaRewardScreen.W_MonsterArenaRewardScreen_C")
hud = unreal.load_asset("/MonsterArena/UI/W_MonsterArenaHUD")
assert hud, "Create the MonsterArena HUD Blueprint first"
hud_class = unreal.load_class(None, "/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD_C")
unreal.get_default_object(hud_class).set_editor_property("reward_screen_class", screen_class)
assert unreal.EditorAssetLibrary.save_loaded_asset(hud, only_if_is_dirty=False)
unreal.log("MONSTER_ARENA_REWARD_SCREEN_SETUP_SUCCEEDED")
