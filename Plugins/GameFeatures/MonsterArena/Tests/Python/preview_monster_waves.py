"""Rendered preview: places the player near the core, leaves waves and input manual."""
import time
import unreal
unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
session = unreal.MonsterArenaRewardTestLibrary
assert session.start_session(1)
started = time.monotonic()
ready = False

def tick(delta):
    global ready
    if ready:
        return
    worlds = session.get_test_worlds()
    if worlds:
        players = unreal.GameplayStatics.get_all_actors_of_class(worlds[0], unreal.LyraPlayerState)
        if players and players[0].get_pawn():
            pawn = players[0].get_pawn()
            pawn.set_actor_location(unreal.Vector(0, 250, 100), False, True)
            pawn.get_controller().set_control_rotation(unreal.Rotator(pitch=0, yaw=90, roll=0))
            ready = True
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.log("MONSTER_ARENA_WAVES_PREVIEW_READY")
    if time.monotonic() - started > 90:
        raise RuntimeError("Preview startup timed out")
handle = unreal.register_slate_post_tick_callback(tick)
