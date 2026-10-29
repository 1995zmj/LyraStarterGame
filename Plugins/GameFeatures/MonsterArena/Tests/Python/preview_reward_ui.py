"""Prepare one rendered PIE player for physical-input and visual reward UI QA.

This is an interactive preview, not an automated gameplay test. It grants three
unclaimed upgrades and disables only this PIE world's monster spawning. It does
not open the menu, pause the game, grant immunity, save assets, or exit the editor.
"""
import time
import traceback
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)

library = unreal.MonsterArenaRewardTestLibrary
started = time.monotonic()
finished = False
experience_granted = False


def prepare_if_ready():
    # All world/player/widget wrappers are local to this call. In particular,
    # never retain the temporary startup world across a tick/travel boundary.
    worlds = library.get_test_worlds()
    if len(worlds) != 1:
        return False
    world = worlds[0]
    if not world.get_name().endswith("L_MonsterArena"):
        return False
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    if not controller:
        return False
    player = controller.get_editor_property("player_state")
    pawn = player.get_pawn() if player else None
    if not player or not pawn:
        return False
    progress = player.get_component_by_class(unreal.RunProgressionComponent)
    health = unreal.LyraHealthComponent.find_health_component(pawn)
    if not progress or not health or health.get_health() <= 0:
        return False
    hud = next(
        (widget for widget in unreal.WidgetLibrary.get_all_widgets_of_class(
            world, unreal.MonsterArenaHUD, False
        ) if widget.get_owning_player() == controller),
        None,
    )
    arenas = [
        actor.get_component_by_class(unreal.MonsterArenaComponent)
        for actor in unreal.GameplayStatics.get_all_actors_of_class(
            world, unreal.GameStateBase
        )
    ]
    if not hud or not any(arenas):
        return False

    # Preview-only world changes: remove distractions for screen/input QA.
    # This deliberately does NOT prove danger, targeting, or damage behavior.
    for arena in arenas:
        if arena:
            arena.set_editor_property("max_alive_monsters", 0)
    for monster in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ArenaMonster):
        monster.destroy_actor()

    global experience_granted
    if not experience_granted:
        experience_granted = True
        progress.add_experience(325)
    assert not hud.is_reward_menu_open(), "Upgrading must not automatically open the menu"
    assert progress.get_editor_property("state").pending_choices == 3, "Expected three pending selections"
    return True


def tick(_delta):
    global finished
    if finished:
        return
    ready = False
    try:
        if time.monotonic() - started > 120.0:
            raise TimeoutError("Reward UI preview did not become ready within 120 seconds")
        ready = prepare_if_ready()
        if not ready:
            return
    except Exception:
        unreal.log_error(traceback.format_exc())
    finished = True
    unreal.unregister_slate_post_tick_callback(handle)
    if ready:
        unreal.log("MONSTER_ARENA_REWARD_UI_PREVIEW_READY")
        unreal.log("Preview: press U / 手柄↑ to open; choose a card and confirm, or return to combat.")
        unreal.log("Preview: monster spawning is disabled in this PIE world only; no immunity was granted.")
    else:
        unreal.log_error("MONSTER_ARENA_REWARD_UI_PREVIEW_FAILED")
    # Keep the editor and PIE running for physical input. The user/test operator
    # ends PIE through the normal editor controls when interactive QA is done.


if not library.start_session(1):
    raise RuntimeError("Could not start preview PIE; finish the existing play session first")
handle = unreal.register_slate_post_tick_callback(tick)
