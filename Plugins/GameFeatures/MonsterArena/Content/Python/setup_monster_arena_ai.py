"""Create the editable monster StateTree once; preserve existing designer edits."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/"], force_rescan=True)
tree = unreal.MonsterArenaStateTreeLibrary.create_monster_state_tree()
assert tree and unreal.MonsterArenaStateTreeLibrary.validate_monster_state_tree(tree)
assert unreal.EditorAssetLibrary.save_loaded_asset(tree, only_if_is_dirty=False)
assert unreal.MonsterArenaStateTreeLibrary.create_monster_state_tree() == tree
assert unreal.MonsterArenaStateTreeLibrary.validate_monster_state_tree(tree)
unreal.log("MONSTER_ARENA_STATETREE_SETUP_SUCCEEDED")
