"""Isolated normal monster for physical gun/AI QA; no immunity or auto-shoot.

Changes only the temporary PIE world. Ends neither PIE nor the editor.
"""
import time
import traceback
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
session = unreal.MonsterArenaRewardTestLibrary
ai = unreal.MonsterArenaAITestLibrary
started = time.monotonic()
prepared = False
fixed_aim = "-MonsterArenaFixedAimPreview" in unreal.SystemLibrary.get_command_line()


def tick(_delta):
    global prepared
    try:
        if not prepared and time.monotonic() - started > 120:
            raise TimeoutError("AI preview startup timeout")
        worlds = session.get_test_worlds()
        if len(worlds) != 1:
            if prepared:
                unreal.unregister_slate_post_tick_callback(handle)
            return
        world = worlds[0]
        controller = unreal.GameplayStatics.get_player_controller(world, 0)
        player = controller.get_editor_property("player_state") if controller else None
        pawn = player.get_pawn() if player else None
        if not pawn or not player.get_component_by_class(unreal.RunProgressionComponent):
            return
        if prepared:
            monsters = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ArenaMonster)
            if not monsters:
                unreal.log("MONSTER_ARENA_AI_PREVIEW_TARGET_REMOVED")
                unreal.unregister_slate_post_tick_callback(handle)
                return
            camera = controller.get_editor_property("player_camera_manager")
            controller.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_camera_location(), monsters[0].get_actor_location()))
            return
        for gs in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.GameStateBase):
            arena = gs.get_component_by_class(unreal.MonsterArenaComponent)
            if arena:
                arena.set_editor_property("max_alive_monsters", 0)
        for monster in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ArenaMonster):
            monster.destroy_actor()
        pawn.set_actor_location(unreal.Vector(-1200, 0, 100), False, True)
        controller.set_control_rotation(unreal.Rotator(0, 0, 0))
        assert ai.spawn_monster(pawn, unreal.Vector(2400, 0, 0))
        prepared = True
        unreal.log("MONSTER_ARENA_AI_PREVIEW_READY: one normal monster, normal health/damage; no automatic firing")
        if fixed_aim:
            unreal.log("Preview fixture: camera continuously aims at monster; physical fire is still required. This does not validate manual mouse aiming.")
            return
    except Exception:
        unreal.log_error(traceback.format_exc())
    unreal.unregister_slate_post_tick_callback(handle)


assert session.start_session(1)
handle = unreal.register_slate_post_tick_callback(tick)
