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
    unreal.log("MonsterArenaStateTree: PASS: " + label)


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


def run_cases():
    worlds = session.get_test_worlds()
    server = next(w for w in worlds if unreal.SystemLibrary.is_server(w))
    client = next(w for w in worlds if not unreal.SystemLibrary.is_server(w))
    players = actors(server, unreal.LyraPlayerState)
    first, second = players[0].get_pawn(), players[1].get_pawn()
    for gs in actors(server, unreal.GameStateBase):
        arena = gs.get_component_by_class(unreal.MonsterArenaComponent)
        if arena:
            arena.set_editor_property("max_alive_monsters", 0)
    for monster in actors(server, unreal.ArenaMonster):
        monster.destroy_actor()
    yield from wait_for(lambda: not session.is_damage_immune(first) and not session.is_damage_immune(second), "Both players leave normal spawn protection")
    first.get_movement_component().disable_movement()
    second.get_movement_component().disable_movement()
    move(first, -500, -500)
    move(second, 1400, 1400)
    monster = ai.spawn_monster(first, unreal.Vector(-800, 0, 0))
    check(monster is not None, "Spawned native monster with configured StateTree")
    check(not monster.is_actor_tick_enabled(), "Monster Actor Tick is disabled")
    yield from wait_for(lambda: ai.get_ai_state(monster) == "Chase" and ai.get_target(monster) == first, "StateTree selects Chase for nearest player")
    initial_distance = monster.get_distance_to(first)
    yield from wait_for(lambda: monster.get_distance_to(first) < initial_distance - 100, "Chase task actually moves the monster")
    yield from wait_for(lambda: any(m.get_name() == monster.get_name() for m in actors(client, unreal.ArenaMonster)), "Monster replicates to client")
    replica = next(m for m in actors(client, unreal.ArenaMonster) if m.get_name() == monster.get_name())
    check(ai.get_ai_state(replica) == "Stopped" and ai.get_target(replica) is None, "Client replica never runs AI or acquires targets")
    yield from wait_for(lambda: replica.get_distance_to(next(p.get_pawn() for p in actors(client, unreal.LyraPlayerState) if p.get_editor_property("player_id") == players[0].get_editor_property("player_id"))) < initial_distance - 80, "Pursuit movement replicates")
    before = health(first)
    other_health = health(second)
    yield from wait_for(lambda: health(first) < before, "Melee state deals real GAS damage")
    check(ai.get_ai_state(monster) == "Melee", "Tree transitions from Chase to Melee")
    after = health(first)
    hit_time = unreal.GameplayStatics.get_time_seconds(server)
    check(abs(before - after - 10) < 0.01, "Melee applies configured ten damage")
    yield from delay(server, 0.35)
    check(health(first) == after, "Melee cooldown prevents per-frame damage")
    yield from wait_for(lambda: health(first) < after, "Melee attacks again after cooldown")
    check(unreal.GameplayStatics.get_time_seconds(server) - hit_time >= 0.9, "Attack interval is at least one second within frame tolerance")
    check(health(second) == other_health, "Non-target player's health is unchanged")

    # Freeze locomotion only; StateTree continues to perceive and transition.
    monster.get_movement_component().disable_movement()
    move(monster, -500, -380)
    blocker = ai.spawn_melee_blocker(monster, first)
    check(blocker is not None, "Created visibility-only melee blocker")
    blocked_health = health(first)
    yield from delay(server, 1.3)
    check(health(first) == blocked_health and ai.get_ai_state(monster) == "Melee", "Melee cannot deal damage through blocker")
    blocker.destroy_actor()
    yield from wait_for(lambda: health(first) < blocked_health, "Removing blocker restores damage")
    move(first, 1300, 1300)
    move(second, -500, 400)
    yield from wait_for(lambda: ai.get_target(monster) == second and ai.get_ai_state(monster) == "Chase", "Nearest living target changes and Melee returns to Chase")
    monster.set_editor_property("detection_radius", 30)
    yield from wait_for(lambda: ai.get_ai_state(monster) == "Idle" and ai.get_target(monster) is None, "No target in detection radius enters Idle")
    check(monster.get_velocity().length() < 0.01, "Idle has no residual movement")
    monster.set_editor_property("detection_radius", 4000)
    yield from wait_for(lambda: ai.get_target(monster) == second, "Idle reacquires a valid player")
    old_second = second
    check(session.deal_damage(second, 10000), "Kill target using normal GAS death flow")
    yield from wait_for(lambda: ai.get_target(monster) == first, "Dead target is rejected and surviving player acquired")
    yield from wait_for(lambda: players[1].get_pawn() and players[1].get_pawn() != old_second, "Target player respawns normally")
    second = players[1].get_pawn()
    second.get_movement_component().disable_movement()
    move(second, -500, 100)
    yield from wait_for(lambda: ai.get_target(monster) == second, "Respawned Pawn can become a new target")
    progression = players[0].get_component_by_class(unreal.RunProgressionComponent)
    kills = progression.get_editor_property("state").kills
    check(session.deal_attributed_damage(first, monster, 10000), "Kill monster using attributed GAS damage")
    check(ai.get_ai_state(monster) == "Stopped" and ai.get_target(monster) is None, "Monster death immediately stops tree and clears target")
    check(progression.get_editor_property("state").kills == kills + 1, "Monster death grants one kill settlement")
    yield from wait_for(lambda: not actors(server, unreal.ArenaMonster) and not actors(client, unreal.ArenaMonster), "Dead monster is destroyed on server and client")


def reentry_cases():
    server = next(w for w in session.get_test_worlds() if unreal.SystemLibrary.is_server(w))
    gs = actors(server, unreal.GameStateBase)[0]
    director = gs.get_component_by_class(unreal.ArenaWaveDirectorComponent)
    player = actors(server, unreal.LyraPlayerState)[0]
    move(player.get_pawn(), 0, 300)
    snapshot = director.get_editor_property("state")
    unreal.MonsterArenaWaveTestLibrary.request_wave(player.get_component_by_class(unreal.RunProgressionComponent), snapshot.run_id, snapshot.wave_index)
    yield from wait_for(lambda: bool(actors(server, unreal.ArenaMonster)), "New session spawns monsters after explicit wave start")
    check(all(ai.get_ai_state(m) in ("Idle", "Chase", "Melee") for m in actors(server, unreal.ArenaMonster)), "New-session monsters start their own StateTree instances")


def run():
    check(session.start_session(2), "Start two-player PIE")
    yield from wait_for(ready, "Both PIE worlds and players are ready", 90)
    yield from run_cases()
    session.stop_session()
    yield from wait_for(lambda: not session.get_test_worlds(), "First session tears down cleanly")
    check(session.start_session(2), "Re-enter two-player match")
    yield from wait_for(ready, "New PIE session players ready", 90)
    yield from reentry_cases()
    session.stop_session()
    yield from wait_for(lambda: not session.get_test_worlds(), "Second session tears down cleanly")


test = run()
finished = False


def tick(_delta):
    global finished
    if finished:
        return
    error = None
    try:
        if time.monotonic() - started > 240:
            raise TimeoutError("AI regression exceeded 240 seconds")
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
    output = os.path.join(unreal.Paths.project_saved_dir(), "Logs", "MonsterArenaStateTreeResult.json")
    with open(output, "w", encoding="utf-8") as stream:
        json.dump({"passed": error is None, "checks": checks, "error": error}, stream, ensure_ascii=False, indent=2)
    unreal.log("MONSTER_ARENA_STATETREE_" + ("SUCCEEDED" if error is None else "FAILED"))
    unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
