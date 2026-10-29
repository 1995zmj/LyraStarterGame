"""Run real listen-server/client PIE reward and UI regressions without saving assets.

Launch the editor with -ExecutePythonScript=<this file>. The asynchronous test
uses normal reflected UI/selection entry points; it does not simulate physical
mouse/keyboard input. A separate interactive pass is required for that coverage.
"""
import json
import os
import time
import traceback
import unreal

unreal.load_module("MonsterArenaRuntime")
unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
library = unreal.MonsterArenaRewardTestLibrary
checks = []
started = time.monotonic()
finished = False


def check(condition, label):
    checks.append({"check": label, "passed": bool(condition)})
    if not condition:
        raise AssertionError(label)
    unreal.log("MonsterArenaRewardNetwork: PASS: " + label)


def wait_for(predicate, label, timeout=60.0):
    deadline = time.monotonic() + timeout
    while not predicate():
        if time.monotonic() >= deadline:
            raise AssertionError("Timed out: " + label)
        yield
    check(True, label)


def delay(seconds):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        yield


def progression(player):
    return player.get_component_by_class(unreal.RunProgressionComponent)


def state(component):
    return component.get_editor_property("state")


def players(world):
    return unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LyraPlayerState)


def ready(world):
    return len(players(world)) == 2 and all(
        p.get_pawn() and progression(p) for p in players(world)
    )


def option_ids(component):
    return [str(card.card_id) for card in state(component).options]


def run_rewards():
    # Retain world wrappers only after client travel completes. A Python
    # reference to the temporary entry world would otherwise prevent its GC.
    worlds = library.get_test_worlds()
    server = next(w for w in worlds if unreal.SystemLibrary.is_server(w))
    client = next(w for w in worlds if not unreal.SystemLibrary.is_server(w))
    server_players = players(server)
    client_pc = unreal.GameplayStatics.get_player_controller(client, 0)
    client_player = client_pc.get_editor_property("player_state")
    client_id = client_player.get_editor_property("player_id")
    server_player = next(p for p in server_players if p.get_editor_property("player_id") == client_id)
    other_player = next(p for p in server_players if p != server_player)
    remote_replica = next(p for p in players(client) if p != client_player)
    authority = progression(server_player)
    check(authority.get_class().get_name() == "B_MonsterArenaProgression_C", "Experience injects the CSV-configured component")
    check(authority.get_editor_property("definition").get_path_name() == "/MonsterArena/Data/DA_MonsterArenaProgression.DA_MonsterArenaProgression", "Live player reads the imported progression asset")
    other = progression(other_player)
    owner = progression(client_player)
    unreal.log("RewardNetwork ownership: server=" + server_player.get_path_name() + " client=" + client_player.get_path_name())

    # Keep combat actors out of this deterministic network test; explicit GAS
    # damage below verifies that the menu grants no protection and handles death.
    for actor in unreal.GameplayStatics.get_all_actors_of_class(server, unreal.GameStateBase):
        arena = actor.get_component_by_class(unreal.MonsterArenaComponent)
        if arena:
            arena.set_editor_property("max_alive_monsters", 0)
    for monster in unreal.GameplayStatics.get_all_actors_of_class(server, unreal.ArenaMonster):
        monster.destroy_actor()

    authority.reset_run()
    other.reset_run()
    authority.add_experience(325)
    other.add_experience(160)
    yield from wait_for(lambda: state(owner).pending_choices == 3, "Owning client receives three accumulated selections")
    check(state(authority).level == 4 and state(authority).experience == 25, "Overflow XP preserved")
    check(not state(owner).options and not state(owner).reward_menu_open, "Level-up neither rolls cards nor opens a menu")
    check(state(progression(remote_replica)).pending_choices == 0, "Other player's private reward state is not replicated to this client")

    def local_hud():
        widgets = unreal.WidgetLibrary.get_all_widgets_of_class(client, unreal.MonsterArenaHUD, False)
        return next((w for w in widgets if w.get_owning_player() == client_pc), None)

    yield from wait_for(lambda: local_hud() is not None, "Owning client's HUD is ready")
    hud = local_hud()
    check(not hud.is_reward_menu_open(), "HUD remains closed after upgrade")
    health = unreal.LyraHealthComponent.find_health_component(server_player.get_pawn())
    yield from wait_for(lambda: not library.is_damage_immune(server_player.get_pawn()), "Normal spawn protection expires", 15.0)
    check(abs(health.get_max_health() - 100.0) < 0.01, "Unclaimed upgrades do not increase health")
    library.open_reward_menu(hud)
    yield from wait_for(lambda: len(state(owner).options) == 3 and hud.is_reward_menu_open(), "Manual UI open requests three server-generated cards")
    ids = option_ids(owner)
    offer_id = state(owner).offer_id
    run_id = state(owner).run_id
    check(len(set(ids)) == 3, "Offered cards are unique")
    def action_snapshot(card):
        return [(action.behavior, action.value) for action in card.actions]
    check([action_snapshot(card) for card in state(owner).options] == [action_snapshot(card) for card in state(authority).options], "Complete behavior lists replicate with the offer")
    check(not unreal.GameplayStatics.is_game_paused(server) and not unreal.GameplayStatics.is_game_paused(client), "Neither world is paused")
    before_time = unreal.GameplayStatics.get_time_seconds(server)
    yield from delay(1.0)
    check(unreal.GameplayStatics.get_time_seconds(server) > before_time + 0.5, "Server world continues ticking while choosing")
    before_health = health.get_health()
    check(library.deal_damage(server_player.get_pawn(), 10.0), "Applied normal GAS damage while menu is open")
    check(abs(health.get_health() - (before_health - 10.0)) < 0.01, "Menu gives no damage immunity")
    library.close_reward_menu(hud)
    yield from wait_for(lambda: not state(authority).reward_menu_open, "Closing UI releases server menu state")
    library.open_reward_menu(hud)
    yield from wait_for(lambda: state(authority).reward_menu_open and state(owner).reward_menu_open and hud.is_reward_menu_open(), "Reopened the same pending selection")
    check(state(owner).offer_id == offer_id and option_ids(owner) == ids, "Closing/reopening cannot reroll")

    library.request_card(owner, run_id, offer_id, "NotInThisOffer")
    yield from delay(0.5)
    check(state(authority).pending_choices == 3 and not state(authority).chosen_rewards, "Invalid client card request grants nothing")
    screen = hud.get_reward_screen()
    library.select_and_confirm_reward(screen, 0)
    yield from wait_for(lambda: state(owner).pending_choices == 2, "UI confirmation grants a card through the client's server RPC")
    unreal.log("RewardNetwork ledger counts: authority=" + str(len(state(authority).chosen_rewards)) + " owner=" + str(len(state(owner).chosen_rewards)))
    unreal.log("RewardNetwork progression objects: authority=" + authority.get_path_name() + " owner=" + owner.get_path_name())
    yield from wait_for(lambda: len(state(owner).chosen_rewards) == 1, "Client reward ledger has arrived")
    check(len(state(authority).chosen_rewards) == 1 and len(state(owner).chosen_rewards) == 1, "Reward ledger replicates to its owner")
    check(action_snapshot(state(owner).chosen_rewards[0].card) == action_snapshot(state(authority).chosen_rewards[0].card), "Granted behavior lists replicate with the reward ledger")
    check(state(other).pending_choices == 2 and not state(other).chosen_rewards, "Other player's rewards are unaffected")
    library.request_card(owner, run_id, offer_id, ids[0])
    yield from delay(0.5)
    check(state(authority).pending_choices == 2 and len(state(authority).chosen_rewards) == 1, "Duplicate stale client confirmation cannot grant twice")

    yield from wait_for(lambda: state(owner).offer_id != offer_id and len(state(owner).options) == 3, "Next selection uses the existing window")
    saved_offer = state(owner).offer_id
    saved_ids = option_ids(owner)
    saved_max_health = health.get_max_health()
    old_pawn = server_player.get_pawn()
    check(library.deal_damage(old_pawn, 10000.0), "Player can die while choosing")
    yield from wait_for(lambda: not hud.is_reward_menu_open(), "Death closes the local reward menu")
    yield from wait_for(lambda: server_player.get_pawn() and server_player.get_pawn() != old_pawn and client_player.get_pawn(), "Normal auto-respawn completes")
    yield from delay(1.0)
    check(state(owner).offer_id == saved_offer and option_ids(owner) == saved_ids and state(owner).pending_choices == 2, "Death preserves the pending offer and count")
    new_health = unreal.LyraHealthComponent.find_health_component(server_player.get_pawn())
    check(abs(new_health.get_max_health() - saved_max_health) < 0.01, "Chosen persistent health survives respawn without duplication")
    check(abs(new_health.get_health() - new_health.get_max_health()) < 0.01, "Normal respawn retains full health while rebuilding rewards")
    check(not hud.is_reward_menu_open(), "Respawn does not automatically reopen rewards")

    library.open_reward_menu(hud)
    yield from wait_for(lambda: hud.is_reward_menu_open() and state(authority).reward_menu_open and state(owner).reward_menu_open, "Player manually resumes pending choices after respawn")
    for remaining in (1, 0):
        # The offer survives death unchanged. A healing-only card may now be
        # inapplicable at full health, so select a permanent/supply alternative.
        index = next(i for i, card in enumerate(state(owner).options) if any(action.behavior in (unreal.RunRewardBehavior.MAX_HEALTH_BONUS, unreal.RunRewardBehavior.DAMAGE_BONUS, unreal.RunRewardBehavior.SPARE_AMMO, unreal.RunRewardBehavior.DAMAGE_PER_KILL) for action in card.actions))
        library.select_and_confirm_reward(hud.get_reward_screen(), index)
        yield from wait_for(lambda: state(owner).pending_choices == remaining, "Consumed one selection, remaining=" + str(remaining))
        yield from delay(0.3)
    yield from wait_for(lambda: not hud.is_reward_menu_open(), "Last selection closes the menu")
    check(not state(authority).reward_menu_open and len(state(authority).chosen_rewards) == 3, "Menu locks released and exactly three grants recorded")
    yield from wait_for(lambda: library.get_input_block_count(server_player.get_pawn()) == 0 and library.get_input_block_count(client_player.get_pawn()) == 0, "Server and owning client release their input blocks")
    check(library.get_input_block_count(other_player.get_pawn()) == 0, "Other player's gameplay input remains unblocked")
    authority.reset_run()
    yield from wait_for(lambda: state(owner).run_id != run_id, "New run invalidates previous selection identifiers")
    library.request_card(owner, run_id, saved_offer, saved_ids[0])
    yield from delay(0.3)
    check(not state(authority).chosen_rewards and state(authority).pending_choices == 0, "Old-run client request cannot grant a reward")
    import runpy
    cases = runpy.run_path(os.path.join(unreal.Paths.project_plugins_dir(), "GameFeatures/MonsterArena/Tests/Python/kill_accumulation_cases.py"))
    yield from cases["run"](authority, owner, server_player, client_player, other_player, hud, library, check, wait_for, delay)



def run():
    check(library.start_session(2), "Started a two-player PIE session")
    yield from wait_for(lambda: len(library.get_test_worlds()) == 2, "Two PIE worlds exist")
    yield from wait_for(lambda: len(library.get_test_worlds()) == 2 and all(ready(w) for w in library.get_test_worlds()), "Both players have pawns and progression components")
    yield from run_rewards()
    # All gameplay wrappers have gone out of scope before world teardown.
    library.stop_session()
    yield from wait_for(lambda: not library.get_test_worlds(), "PIE shuts down cleanly")


test = run()


def tick(_delta):
    global finished
    if finished:
        return
    error = None
    try:
        if time.monotonic() - started > 180:
            raise TimeoutError("Network regression exceeded 180 seconds")
        next(test)
        return
    except StopIteration:
        pass
    except Exception:
        error = traceback.format_exc()
        unreal.log_error(error)
        test.close()
        library.stop_session()
    finished = True
    unreal.unregister_slate_post_tick_callback(handle)
    result = {"passed": error is None, "checks": checks, "error": error}
    output = os.path.join(unreal.Paths.project_saved_dir(), "Logs", "MonsterArenaRewardNetworkResult.json")
    with open(output, "w", encoding="utf-8") as stream:
        json.dump(result, stream, ensure_ascii=False, indent=2)
    unreal.log("MONSTER_ARENA_REWARD_NETWORK_" + ("SUCCEEDED" if error is None else "FAILED"))
    unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
