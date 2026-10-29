"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.

Creates MonsterArena assets and scopes shared relay registration to shooter
experiences. Existing Blueprint graphs and custom experience actions are preserved.
"""
import unreal
import runpy
from pathlib import Path

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
assert unreal.MonsterArenaContentLibrary.static_class().get_path_name() == "/Script/MonsterArenaEditor.MonsterArenaContentLibrary"
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MonsterArena/", "/ShooterCore/"], force_rescan=True)
tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary


def data_asset(path, cls):
    existing = unreal.load_asset(path)
    if existing:
        return existing
    folder, name = path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    result = tools.create_asset(name, folder, cls, factory)
    assert result, path
    return result


feature = data_asset("/MonsterArena/MonsterArena", unreal.GameFeatureData)
# Clone scan rules to retain Lyra's supported primary asset type configuration.
source = unreal.load_asset("/ShooterCore/ShooterCore")
scan_rules = source.get_editor_property("primary_asset_types_to_scan")
scan_rules = [rule for rule in scan_rules if str(rule.get_editor_property("primary_asset_type")) != "PlayerMappableInputConfig"]
for rule in scan_rules:
    kind = str(rule.get_editor_property("primary_asset_type"))
    directory = {"Map": "Maps", "LyraExperienceDefinition": "System/Experiences", "LyraExperienceActionSet": "System/Experiences", "LyraUserFacingExperienceDefinition": "System/Playlists"}[kind]
    rule.set_editor_property("directories", [unreal.DirectoryPath(path=directory)])
feature.set_editor_property("primary_asset_types_to_scan", scan_rules)
assets.save_loaded_asset(feature)

# These Blueprint assets contain the MonsterArena-specific safety checks and are
# versioned with the plugin. Never regenerate them from unmodified shared widgets.
for path in [
    "/MonsterArena/UI/Notifications/B_MonsterArenaEliminationFeedRelay",
    "/MonsterArena/UI/Weapons/W_MonsterArenaReticle_Rifle",
    "/MonsterArena/UI/Weapons/W_MonsterArenaReticle_Pistol",
]:
    assert assets.does_asset_exist(path), "Restore the plugin's versioned UI asset: " + path

for source_path, target_path in [
    ("/ShooterCore/Weapons/Rifle/ID_Rifle", "/MonsterArena/Weapons/Rifle/ID_MonsterArenaRifle"),
    ("/ShooterCore/Weapons/Pistol/ID_Pistol", "/MonsterArena/Weapons/Pistol/ID_MonsterArenaPistol"),
    ("/ShooterCore/Game/HeroData_ShooterGame", "/MonsterArena/System/HeroData_MonsterArena"),
    ("/ShooterCore/Experiences/LAS_ShooterGame_StandardComponents", "/MonsterArena/System/Experiences/LAS_MonsterArena_Components"),
]:
    if not assets.does_asset_exist(target_path):
        assert assets.duplicate_asset(source_path, target_path)

# Shared weapon instances cast to B_Hero_ShooterMannequin. A child preserves that
# contract; a duplicate with the same native parent does not.
hero_path = "/MonsterArena/Characters/B_MonsterArenaShooterHero"
if not assets.does_asset_exist(hero_path):
    hero_factory = unreal.BlueprintFactory()
    hero_factory.set_editor_property("parent_class", unreal.load_class(None, "/ShooterCore/Game/B_Hero_ShooterMannequin.B_Hero_ShooterMannequin_C"))
    assert tools.create_asset("B_MonsterArenaShooterHero", "/MonsterArena/Characters", unreal.Blueprint, hero_factory)

for kind in ["Rifle", "Pistol"]:
    path = "/MonsterArena/Weapons/{0}/ID_MonsterArena{0}".format(kind)
    item = unreal.load_asset(path)
    cdo = unreal.get_default_object(item.generated_class())
    for fragment in cdo.get_editor_property("fragments"):
        if "InventoryFragment_ReticleConfig" in fragment.get_class().get_name():
            widgets = list(fragment.get_editor_property("reticle_widgets"))
            for index, widget in enumerate(widgets):
                if "W_Reticle_" + kind in widget.get_path_name():
                    widgets[index] = unreal.load_class(None, "/MonsterArena/UI/Weapons/W_MonsterArenaReticle_{0}.W_MonsterArenaReticle_{0}_C".format(kind))
            fragment.set_editor_property("reticle_widgets", widgets)
    unreal.BlueprintEditorLibrary.compile_blueprint(item)
    assets.save_loaded_asset(item)

hero = unreal.load_asset(hero_path)
hero_defaults = unreal.get_default_object(hero.generated_class())
inventory = list(hero_defaults.get_editor_property("InitialInventoryItems"))
for index, item_class in enumerate(inventory):
    for kind in ["Rifle", "Pistol"]:
        if "/ShooterCore/Weapons/{0}/ID_{0}.".format(kind) in item_class.get_path_name():
            inventory[index] = unreal.load_class(None, "/MonsterArena/Weapons/{0}/ID_MonsterArena{0}.ID_MonsterArena{0}_C".format(kind))
# Keep the starting rifle as a hard reference for cooking as well as quickbar setup.
rifle_class = unreal.load_class(None, "/MonsterArena/Weapons/Rifle/ID_MonsterArenaRifle.ID_MonsterArenaRifle_C")
if rifle_class not in inventory:
    inventory.append(rifle_class)
hero_defaults.set_editor_property("InitialInventoryItems", inventory)
unreal.BlueprintEditorLibrary.compile_blueprint(hero)
assets.save_loaded_asset(hero)
pawn_data = unreal.load_asset("/MonsterArena/System/HeroData_MonsterArena")
pawn_data.set_editor_property("pawn_class", hero.generated_class())
assets.save_loaded_asset(pawn_data)
unreal.MonsterArenaContentLibrary.configure_editor_asset_scope()
for path in ["/ShooterCore/ShooterCore", "/ShooterCore/Experiences/LAS_ShooterGame_StandardComponents", "/MonsterArena/System/Experiences/LAS_MonsterArena_Components"]:
    assert assets.save_asset(path)

experience_path = "/MonsterArena/System/Experiences/B_MonsterArena"
experience = unreal.load_asset(experience_path)
if not experience:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.LyraExperienceDefinition)
    experience = tools.create_asset("B_MonsterArena", "/MonsterArena/System/Experiences", unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(experience)
    cdo = unreal.get_default_object(experience.generated_class())
    unreal.MonsterArenaContentLibrary.configure_experience(cdo)
    unreal.BlueprintEditorLibrary.compile_blueprint(experience)
    assets.save_loaded_asset(experience)

# Update only shared defaults being replaced; preserve custom experience actions.
defaults = unreal.get_default_object(experience.generated_class())
defaults.set_editor_property("default_pawn_data", pawn_data)
action_sets = list(defaults.get_editor_property("action_sets"))
for index, action_set in enumerate(action_sets):
    if action_set.get_path_name().startswith("/ShooterCore/Experiences/LAS_ShooterGame_StandardComponents."):
        action_sets[index] = unreal.load_asset("/MonsterArena/System/Experiences/LAS_MonsterArena_Components")
defaults.set_editor_property("action_sets", action_sets)
unreal.BlueprintEditorLibrary.compile_blueprint(experience)
assets.save_loaded_asset(experience)

map_path = "/MonsterArena/Maps/L_MonsterArena"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if assets.does_asset_exist(map_path):
    assert levels.load_level(map_path)
else:
    assert levels.new_level(map_path)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not any(actor.get_actor_label() == "ArenaFloor" for actor in actors.get_all_level_actors()):
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -50))
    floor.set_actor_label("ArenaFloor")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(40, 40, 1))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-50, -30, 0))
    sun.light_component.set_editor_property("intensity", 5.0)
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 600))
    # Lyra's spawn management discovers LyraPlayerStart, not a generic PlayerStart.
    start_class = unreal.load_class(None, "/Script/LyraGame.LyraPlayerStart")
    for y in [-150, 150]:
        actors.spawn_actor_from_class(start_class, unreal.Vector(0, y, 100))
    for x, y in [(1000, 0), (-1000, 0), (0, 1000), (0, -1000)]:
        point = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, 100))
        point.set_actor_label("MonsterSpawn")
        point.set_editor_property("tags", ["MonsterSpawn"])
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.MonsterArenaContentLibrary.configure_world(world)
assert levels.save_current_level()

playlist = data_asset("/MonsterArena/System/Playlists/DA_MonsterArena", unreal.LyraUserFacingExperienceDefinition)
# UWorld primary asset names use the full package path, not the short asset name.
playlist.set_editor_property("map_id", unreal.PrimaryAssetId(unreal.PrimaryAssetType("Map"), map_path))
playlist.set_editor_property("experience_id", unreal.PrimaryAssetId(unreal.PrimaryAssetType("LyraExperienceDefinition"), "B_MonsterArena"))
playlist.set_editor_property("tile_title", "Monster Arena")
playlist.set_editor_property("tile_sub_title", "Shoot monsters. Level up. Get stronger.")
playlist.set_editor_property("max_player_count", 2)
assets.save_loaded_asset(playlist)

# Restore CSV-authored progression binding after experience configuration.
runpy.run_path(str(Path(__file__).with_name("import_progression_csv.py")), run_name="__main__")
unreal.log("MONSTER_ARENA_SETUP_SUCCEEDED")
