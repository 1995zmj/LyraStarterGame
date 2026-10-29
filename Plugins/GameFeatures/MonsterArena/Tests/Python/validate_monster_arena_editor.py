"""Validate the editor scripting bridge without changing saved project assets."""
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")

library = unreal.MonsterArenaContentLibrary
assert library.static_class().get_path_name() == "/Script/MonsterArenaEditor.MonsterArenaContentLibrary"
assert unreal.find_object(None, "/Script/MonsterArenaRuntime.MonsterArenaContentLibrary") is None

# Exercise the reflected function on a transient asset, leaving saved actions intact.
probe = unreal.new_object(unreal.LyraExperienceDefinition)
library.configure_experience(probe)
assert list(probe.get_editor_property("game_features_to_enable")) == ["ShooterCore", "MonsterArena"]
assert probe.get_editor_property("default_pawn_data").get_path_name() == "/MonsterArena/System/HeroData_MonsterArena.HeroData_MonsterArena"
assert len(probe.get_editor_property("actions")) == 2

unreal.log("MONSTER_ARENA_EDITOR_MODULE_VALIDATION_SUCCEEDED")
