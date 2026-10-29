"""Additional live two-player cases, invoked by validate_reward_multiplayer.py."""
import unreal


def run(authority, owner, server_player, client_player, victim, hud, library, check, wait_for, delay):
    def state(component):
        return component.get_editor_property("state")

    def accumulated():
        return state(authority).accumulated_kill_damage

    def damage():
        return library.get_base_damage(server_player.get_pawn())

    def claim():
        yield from wait_for(lambda: state(owner).pending_choices > 0, "Accumulation choice reaches owning client")
        library.open_reward_menu(hud)
        yield from wait_for(lambda: hud.is_reward_menu_open() and len(state(owner).options) == 1, "CSV accumulation card is available at full health")
        check(str(state(owner).options[0].card_id) == "FirepowerAccumulation", "The offered card comes from the imported CSV")
        library.select_and_confirm_reward(hud.get_reward_screen(), 0)
        yield from wait_for(lambda: not hud.is_reward_menu_open(), "Accumulation card confirmed through client RPC")

    def respawn(player, old):
        yield from wait_for(lambda: player.get_pawn() and player.get_pawn() != old, "Player receives a new pawn")
        yield from wait_for(lambda: not library.is_damage_immune(player.get_pawn()), "Respawn protection expires before combat", 15.0)
        yield from delay(0.4)

    authority.reset_run()
    check(library.use_accumulation_pool(authority), "Test pool uses the saved CSV accumulation entry")
    baseline = damage()
    check(library.kill_test_monster(server_player.get_pawn()), "Real monster death and duplicate settlement exercised before claiming")
    check(accumulated() == 0 and damage() == baseline, "Kills before claiming are not retroactive")
    authority.add_experience(60)
    yield from claim()
    check(authority.get_damage_per_kill() == 1 and accumulated() == 0 and damage() == baseline, "First card registers a listener without an immediate damage grant")
    check(library.kill_test_monster(server_player.get_pawn()), "Monster settlement is credited once")
    check(accumulated() == 1 and damage() == baseline + 1, "One monster grants exactly one damage")
    authority.add_experience(100)
    yield from claim()
    check(authority.get_damage_per_kill() == 2 and accumulated() == 1, "Second card raises future yield without replaying old kills")
    check(library.kill_test_monster(server_player.get_pawn()), "Second monster killed with two stacked cards")
    check(accumulated() == 3 and damage() == baseline + 3, "Two stacked cards grant two damage per kill")

    check(not library.set_test_team(server_player.get_pawn(), 0), "Unregistered test team is rejected without mutating player")
    check(library.set_test_team(server_player.get_pawn(), 1), "Killer joins registered red team")
    check(library.set_test_team(victim.get_pawn(), 2), "Victim joins registered blue team")
    yield from wait_for(lambda: not library.is_damage_immune(victim.get_pawn()), "Enemy player is vulnerable")
    old_victim = victim.get_pawn()
    check(library.deal_attributed_damage(server_player.get_pawn(), old_victim, 10000), "Attributed GAS damage kills an enemy player")
    check(accumulated() == 5 and damage() == baseline + 5, "Enemy player elimination grants the same stacked yield")
    library.repeat_elimination(server_player.get_pawn(), old_victim)
    check(accumulated() == 5, "Repeated player elimination notification grants nothing")
    yield from wait_for(lambda: state(owner).accumulated_kill_damage == 5 and library.get_base_damage(client_player.get_pawn()) == baseline + 5, "Accumulated state and actual damage replicate to the owning client")
    yield from respawn(victim, old_victim)

    # Friendly fire is explicitly applied through GAS to test the reward filter.
    check(library.set_test_team(victim.get_pawn(), 1), "Friendly-fire victim joins registered red team")
    old_victim = victim.get_pawn()
    check(library.deal_attributed_damage(server_player.get_pawn(), old_victim, 10000), "Friendly-fire death exercised")
    check(accumulated() == 5, "Teammate elimination is excluded")
    yield from respawn(victim, old_victim)
    check(library.set_test_team(victim.get_pawn(), 2), "Victim joins registered blue team")

    old_owner = server_player.get_pawn()
    check(library.deal_damage(old_owner, 10000), "Owner suicide exercised")
    check(accumulated() == 5, "Suicide is excluded")
    yield from respawn(server_player, old_owner)
    check(accumulated() == 5 and damage() == baseline + 5 and authority.get_damage_per_kill() == 2, "Respawn preserves accumulated damage and future yield without duplication")

    old_victim = victim.get_pawn()
    check(library.deal_attributed_damage(server_player.get_pawn(), old_victim, 10), "Nonlethal contribution exercised")
    check(accumulated() == 5, "Nonlethal damage grants no kill reward")
    check(library.deal_attributed_damage(server_player.get_pawn(), old_victim, 10000), "Same enemy player killed after its respawn")
    check(accumulated() == 7 and damage() == baseline + 7, "New victim pawn is credited and owner can accumulate after respawn")
    authority.reset_run()
    check(accumulated() == 0 and authority.get_damage_per_kill() == 0 and damage() == baseline, "New run removes both earned damage and trigger stacks")
    library.repeat_elimination(server_player.get_pawn(), old_victim)
    check(state(authority).kills == 0 and accumulated() == 0, "Reset does not let the old corpse award another kill")
    yield from wait_for(lambda: state(owner).accumulated_kill_damage == 0, "Reset state replicates to client")
