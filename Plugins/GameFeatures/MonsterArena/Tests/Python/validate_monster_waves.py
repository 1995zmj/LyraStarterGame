"""Rendered two-player PIE: real StateTree movement, damage and lifecycle.

Uses reflected test setup, not physical input. Does not save preview worlds.
"""
import json
import os
import time
import traceback
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
session = unreal.MonsterArenaRewardTestLibrary
ai = unreal.MonsterArenaAITestLibrary
checks = []
started = time.monotonic()


def check(value, label):
    checks.append({"check": label, "passed": bool(value)})
    if not value:
        raise AssertionError(label)
    unreal.log("MonsterArenaWaves: PASS: " + label)


def wait_for(predicate, label, timeout=30):
    deadline = time.monotonic() + timeout
    while not predicate():
        if time.monotonic() > deadline:
            raise TimeoutError(label)
        yield
    check(True, label)


def delay(world, seconds):
    deadline = unreal.GameplayStatics.get_time_seconds(world) + seconds
    yield from wait_for(lambda: unreal.GameplayStatics.get_time_seconds(world) >= deadline, "Elapsed game time " + str(seconds))


def actors(world, cls):
    return unreal.GameplayStatics.get_all_actors_of_class(world, cls)


def ready():
    worlds = session.get_test_worlds()
    return len(worlds) == 2 and all(
        len(actors(w, unreal.LyraPlayerState)) == 2 and all(p.get_pawn() for p in actors(w, unreal.LyraPlayerState))
        for w in worlds)


def health(pawn):
    return unreal.LyraHealthComponent.find_health_component(pawn).get_health()


def move(actor, x, y, z=100):
    actor.set_actor_location(unreal.Vector(x, y, z), False, True)


wave_tools = unreal.MonsterArenaWaveTestLibrary
Phase = unreal.ArenaWavePhase


def director(world):
    gs = actors(world, unreal.GameStateBase)[0]
    return gs.get_component_by_class(unreal.ArenaWaveDirectorComponent)


def state(d):
    return d.get_editor_property("state")


def run():
    check(session.start_session(2), "Start two-player PIE")
    yield from wait_for(ready, "Both PIE worlds ready", 90)
    server = next(w for w in session.get_test_worlds() if unreal.SystemLibrary.is_server(w))
    client = next(w for w in session.get_test_worlds() if not unreal.SystemLibrary.is_server(w))
    yield from wait_for(lambda: director(server) and director(client), "Director replicated to client")
    d, cd = director(server), director(client)
    check(d.get_editor_property("core") is not None, "Authored defense core is present")
    check(state(d).phase == Phase.PREPARATION and not actors(server, unreal.ArenaMonster), "No automatic spawning during preparation")
    check(len(d.get_editor_property("definition").get_editor_property("waves")) == 5, "Five authored wave configs loaded")
    owner = next(p for p in actors(client, unreal.LyraPlayerState) if p.get_pawn().is_locally_controlled())
    owner_id = owner.get_editor_property("player_id")
    sp = next(p for p in actors(server, unreal.LyraPlayerState) if p.get_editor_property("player_id") == owner_id)
    progress = owner.get_component_by_class(unreal.RunProgressionComponent)
    for ps in actors(server, unreal.LyraPlayerState):
        move(ps.get_pawn(), -800, -700)
    wave_tools.use_short_waves(d)
    yield from wait_for(lambda: state(cd).run_id == state(d).run_id, "Reset replicated")
    wave_tools.request_wave(progress, state(d).run_id, state(d).wave_index)
    yield from delay(server, 0.5)
    check(state(d).phase == Phase.PREPARATION, "Remote start rejected outside interaction radius")
    move(sp.get_pawn(), 0, 300)
    yield from delay(server, 0.5)
    run_id = state(d).run_id
    wave_tools.request_wave(progress, run_id - 1, 0)
    yield from delay(server, 0.3)
    check(state(d).wave_index == 0, "Stale run request rejected")
    wave_tools.request_wave(progress, run_id, 0)
    wave_tools.request_wave(progress, run_id, 0)
    yield from wait_for(lambda: state(d).alive_count == 2, "First batch reaches alive cap")
    check(state(d).wave_index == 1, "Duplicate start cannot skip waves")
    for m in actors(server, unreal.ArenaMonster):
        m.get_movement_component().disable_movement()
    yield from delay(server, 1.2)
    check(state(d).alive_count == 2 and state(d).pending_count == 2, "Cap retains pending quota without burst spawning")
    yield from wait_for(lambda: state(cd).alive_count == 2 and state(cd).pending_count == 2, "Wave counters replicate")
    removed = actors(server, unreal.ArenaMonster)[0]
    removed.destroy_actor()
    check(state(d).alive_count == 1 and state(d).pending_count == 3, "Abnormal destruction requeues exactly one enemy")
    yield from wait_for(lambda: state(d).alive_count == 2, "Requeued enemy respawns")
    for wave in range(1, 6):
        deadline = time.monotonic() + 40
        while state(d).phase not in (Phase.INTERMISSION, Phase.VICTORY):
            assert state(d).alive_count <= 2, "Wave alive limit exceeded"
            for m in actors(server, unreal.ArenaMonster):
                if unreal.LyraHealthComponent.find_health_component(m).get_health() > 0:
                    session.deal_attributed_damage(sp.get_pawn(), m, 10000)
            if time.monotonic() > deadline:
                raise TimeoutError("Wave clear " + str(wave))
            yield
        check(state(d).wave_index == wave and state(d).pending_count == 0 and state(d).alive_count == 0, "Wave %d cleared exactly" % wave)
        if wave < 5:
            yield from delay(server, 0.5)
            check(state(d).phase == Phase.INTERMISSION, "Intermission requires explicit input")
            move(sp.get_pawn(), 0, 300)
            wave_tools.request_wave(progress, run_id, wave)
            yield from wait_for(lambda: state(d).wave_index == wave + 1, "Next wave accepted from owner client")
    check(state(d).phase == Phase.VICTORY, "Fifth clear wins")
    yield from wait_for(lambda: state(cd).phase == Phase.VICTORY, "Victory replicates")
    wave_tools.request_wave(progress, run_id, 5)
    yield from wait_for(lambda: state(d).run_id != run_id and state(d).phase == Phase.PREPARATION, "Client restarts cleanly")
    check(sp.get_component_by_class(unreal.RunProgressionComponent).get_editor_property("state").kills == 0, "New run resets player progression")
    run_id = state(d).run_id
    # Validate sustained spawn failure retains the full quota, then recovers.
    original_class = d.get_editor_property("monster_class")
    d.set_editor_property("monster_class", None)
    wave_tools.request_wave(progress, run_id, 0)
    yield from wait_for(lambda: state(d).spawn_blocked, "Persistent spawn failure is visible", 20)
    check(state(d).pending_count == 4 and state(d).alive_count == 0, "Failed spawns never consume quota or win")
    d.set_editor_property("monster_class", original_class)
    yield from wait_for(lambda: state(d).alive_count > 0 and not state(d).spawn_blocked, "Spawner recovers after configuration restored")
    # Free a slot, kill both players, and verify pending spawns continue during respawn.
    old_pawns = [ps.get_pawn() for ps in actors(server, unreal.LyraPlayerState)]
    for m in actors(server, unreal.ArenaMonster):
        if unreal.LyraHealthComponent.find_health_component(m).get_health() > 0:
            session.deal_attributed_damage(sp.get_pawn(), m, 10000)
    pending_before_death = state(d).pending_count
    for pawn in old_pawns:
        check(session.deal_damage(pawn, 10000), "Apply real player death")
    yield from wait_for(lambda: state(d).pending_count < pending_before_death, "Spawner continues while players await respawn", 8)
    check(state(d).phase != Phase.DEFEAT, "Player deaths do not end core-defense run")
    yield from wait_for(lambda: all(ps.get_pawn() and ps.get_pawn() not in old_pawns for ps in actors(server, unreal.LyraPlayerState)), "Both players respawn", 20)
    # Let real StateTree enemies navigate and damage the core without manual damage.
    for ps in actors(server, unreal.LyraPlayerState):
        move(ps.get_pawn(), -1500, -1500)
    core = d.get_editor_property("core")
    core_health = core.get_editor_property("health")
    yield from wait_for(lambda: core.get_editor_property("health") < core_health, "Assault enemy navigates to and damages core", 40)
    wave_tools.damage_core(d, 10000)
    check(state(d).phase == Phase.DEFEAT and state(d).alive_count == 0 and state(d).pending_count == 0, "Core destruction fails run and clears spawner")
    yield from delay(server, 1)
    check(not actors(server, unreal.ArenaMonster), "No late spawns after defeat")
    yield from wait_for(lambda: state(cd).phase == Phase.DEFEAT, "Defeat replicates")
    session.stop_session()
    yield from wait_for(lambda: not session.get_test_worlds(), "Session teardown")
    check(session.start_session(2), "Re-enter match")
    yield from wait_for(ready, "Re-entry ready", 90)
    server = next(w for w in session.get_test_worlds() if unreal.SystemLibrary.is_server(w))
    yield from wait_for(lambda: director(server) is not None, "New director ready")
    check(state(director(server)).phase == Phase.PREPARATION and state(director(server)).wave_index == 0 and not actors(server, unreal.ArenaMonster), "Re-entry has no old wave or monsters")
    session.stop_session()
    yield from wait_for(lambda: not session.get_test_worlds(), "Final teardown")

test = run()
finished = False


def tick(_delta):
    global finished
    if finished:
        return
    error = None
    try:
        if time.monotonic() - started > 360:
            raise TimeoutError("AI regression exceeded 360 seconds")
        next(test)
        return
    except StopIteration:
        pass
    except Exception:
        error = traceback.format_exc()
        unreal.log_error(error)
        test.close()
        session.stop_session()
    finished = True
    unreal.unregister_slate_post_tick_callback(handle)
    output = os.path.join(unreal.Paths.project_saved_dir(), "Logs", "MonsterArenaWavesResult.json")
    with open(output, "w", encoding="utf-8") as stream:
        json.dump({"passed": error is None, "checks": checks, "error": error}, stream, ensure_ascii=False, indent=2)
    unreal.log("MONSTER_ARENA_WAVES_" + ("SUCCEEDED" if error is None else "FAILED"))
    unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)

