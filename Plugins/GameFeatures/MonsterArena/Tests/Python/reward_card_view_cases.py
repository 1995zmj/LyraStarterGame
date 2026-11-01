"""Checks real child card/row widgets without sending a gameplay request."""
import unreal


def validate(screen, owner, check):
    options = owner.get_editor_property("state").options
    cards = screen.get_editor_property("cards")
    check(len(cards) == 3, "Reward screen contains three reusable card views")
    for card, option in zip(cards, options):
        check(card.get_class().get_path_name() == "/MonsterArena/UI/W_MonsterArenaRewardCard.W_MonsterArenaRewardCard_C", "Candidate uses the standalone card Blueprint")
        check(str(card.get_editor_property("displayed_card_id")) == str(option.card_id), "Card receives the candidate snapshot")
        validate_rows(card, option, check)

    # Use one view to test all behaviors and a repeated action, then restore it.
    combined = unreal.RunRewardOption()
    combined.set_editor_property("card_id", "ViewCompositionTest")
    actions = []
    for kind, value in ((unreal.RunRewardBehavior.MAX_HEALTH_BONUS, 10),
                        (unreal.RunRewardBehavior.DAMAGE_BONUS, 1),
                        (unreal.RunRewardBehavior.HEAL_AMOUNT, 5),
                        (unreal.RunRewardBehavior.HEAL_FRACTION, 0.4),
                        (unreal.RunRewardBehavior.SPARE_AMMO, 60),
                        (unreal.RunRewardBehavior.DAMAGE_PER_KILL, 1),
                        (unreal.RunRewardBehavior.DAMAGE_BONUS, 2)):
        action = unreal.RunRewardAction()
        action.set_editor_property("behavior", kind)
        action.set_editor_property("value", value)
        actions.append(action)
    combined.set_editor_property("actions", actions)
    ledger_count = len(owner.get_editor_property("state").chosen_rewards)
    card = cards[0]
    try:
        card.configure(combined, 0, 0, owner)
        validate_rows(card, combined, check)
        check(len(card.get_editor_property("effect_rows")) == 6, "Seven action records produce six merged effect rows")
        percent = next(row for row in card.get_editor_property("effect_rows") if row.get_editor_property("behavior") == unreal.RunRewardBehavior.HEAL_FRACTION)
        check(str(percent.get_editor_property("effect_value").get_text()) == "+40%", "Fractional healing displays as forty percent")
    finally:
        original = options[0]
        stacks = sum(str(grant.card.card_id) == str(original.card_id) for grant in owner.get_editor_property("state").chosen_rewards)
        card.configure(original, 0, stacks, owner)
    check(len(owner.get_editor_property("state").chosen_rewards) == ledger_count, "Configuring a card view cannot grant a reward")


def validate_rows(card, option, check):
    expected = {}
    for action in option.actions:
        key = str(action.behavior)
        expected[key] = expected.get(key, 0) + action.value
    rows = card.get_editor_property("effect_rows")
    check(len(rows) == len(expected), "Effect rows match the behavior list without duplicated stale rows")
    for row in rows:
        check(row.get_class().get_path_name() == "/MonsterArena/UI/W_MonsterArenaRewardStat.W_MonsterArenaRewardStat_C", "Effect uses the reusable stat Blueprint")
        key = str(row.get_editor_property("behavior"))
        check(abs(row.get_editor_property("value") - expected[key]) < 0.00001, "Stat row receives the summed action value")
