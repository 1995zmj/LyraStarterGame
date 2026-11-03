"""Rendered two-player PIE: authority health -> replicated core -> real HUD text."""
import time
import traceback
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
session = unreal.MonsterArenaRewardTestLibrary
started = time.monotonic()


def wait(predicate, label):
    deadline = time.monotonic() + 90
    while not predicate():
        if time.monotonic() > deadline:
            raise TimeoutError(label)
        yield
    unreal.log("BaseHealthHUD PASS: " + label)


def director(world):
    states = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.GameStateBase)
    return states[0].get_component_by_class(unreal.ArenaWaveDirectorComponent) if states else None


def text(world):
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    widgets = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.MonsterArenaHUD, False)
    hud = next((w for w in widgets if w.get_owning_player() == pc), None)
    return str(hud.get_editor_property("status").get_text()) if hud else ""


def run():
    for attempt in range(2):
        assert session.start_session(2)
        yield from wait(lambda: len(session.get_test_worlds()) == 2 and all(director(w) and director(w).get_editor_property("core") for w in session.get_test_worlds()), "Both cores ready")
        worlds = session.get_test_worlds()
        server = next(w for w in worlds if unreal.SystemLibrary.is_server(w))
        d = director(server)
        core = d.get_editor_property("core")
        yield from wait(lambda: all(text(w).partition("\n")[0] == "大本营血量：1,000 / 1,000" for w in worlds), "Initial HUD and re-entry")
        # Nondefault max verifies replication rather than matching client defaults.
        core.set_editor_property("max_health", 1500.0)
        d.reset_run()
        unreal.MonsterArenaWaveTestLibrary.damage_core(d, 250)
        yield from wait(lambda: all(text(w).partition("\n")[0] == "大本营血量：1,250 / 1,500" for w in worlds), "Damage and custom max replicated to both HUDs")
        unreal.SystemLibrary.execute_console_command(server, "Shot SHOWUI")
        for _ in range(20):
            yield
        unreal.MonsterArenaWaveTestLibrary.damage_core(d, 10000)
        yield from wait(lambda: all(text(w).partition("\n")[0] == "大本营血量：0 / 1,500" for w in worlds), "Destroyed core displays zero")
        d.reset_run()
        yield from wait(lambda: all(text(w).partition("\n")[0] == "大本营血量：1,500 / 1,500" for w in worlds), "Reset restores HUD")
        session.stop_session()
        yield from wait(lambda: not session.get_test_worlds(), "PIE teardown")


test = run()


def tick(_delta):
    try:
        if time.monotonic() - started > 240:
            raise TimeoutError("HUD test timeout")
        next(test)
        return
    except StopIteration:
        unreal.log("MONSTER_ARENA_BASE_HEALTH_HUD_PASSED")
    except Exception:
        unreal.log_error(traceback.format_exc())
        session.stop_session()
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)

