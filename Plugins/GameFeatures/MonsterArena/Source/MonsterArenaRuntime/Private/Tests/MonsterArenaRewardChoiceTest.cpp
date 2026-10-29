#include "MonsterArenaRewardTestHelpers.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Player/LyraPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayEffect.h"

#if WITH_DEV_AUTOMATION_TESTS
// Runs against the initialized player's real ASC and inventory as part of the combat smoke test.
bool RunMonsterArenaRewardChoiceTests(ALyraPlayerState* Player)
{
	using namespace MonsterArenaRewardTests;
	URunProgressionComponent* Progress = Player ? Player->FindComponentByClass<URunProgressionComponent>() : nullptr;
	ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	if (!Progress || !ASC || !Player->GetPawn() || !Player->GetPlayerController())
	{
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaRewardTest: FAIL: initialized player is required"));
		return false;
	}
	bool bPassed = true;
	auto Verify = [&bPassed](bool bCondition, const TCHAR* Description)
	{
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaRewardTest: %s: %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), Description);
		bPassed &= bCondition;
	};
	FScopedPool Pool(Progress, {
		Card(TEXT("TestFirepower"), 0.0f, 3.0f),
		Card(TEXT("TestVitality"), 20.0f, 0.0f, 20.0f),
		Card(TEXT("TestBalance"), 10.0f, 1.0f, 10.0f)});
	Progress->ResetRun();
	const float BaseMaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
	const float BaseDamage = ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute());
	Progress->AddExperience(160);
	Verify(Progress->State.PendingChoices == 2 && Progress->State.ChosenRewards.IsEmpty()
		&& Progress->State.Options.IsEmpty() && !Progress->State.bRewardMenuOpen,
		TEXT("Two upgrades remain unclaimed until the player manually opens rewards"));
	Progress->RequestOpenRewardChoices();
	const int32 FirstRun = Progress->State.RunId;
	const int32 FirstOffer = Progress->State.OfferId;
	const TArray<FName> FirstCards = CardIds(Progress);
	TSet<FName> UniqueCards;
	for (FName Id : FirstCards)
	{
		UniqueCards.Add(Id);
	}
	Verify(Progress->State.bRewardMenuOpen && FirstCards.Num() == 3 && UniqueCards.Num() == 3,
		TEXT("Manual opening offers three distinct cards"));
	Verify(!UGameplayStatics::IsGamePaused(Player), TEXT("Opening rewards does not pause the world"));
	Verify(ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked), TEXT("Open reward menu blocks gameplay ability input"));
	Verify(!ASC->HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity), TEXT("Opening rewards grants no damage immunity"));

	// Damage goes through the same health execution path as monster melee while selection is open.
	const float InjuredHealth = BaseMaxHealth * 0.6f;
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), InjuredHealth);
	UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>(Progress);
	FGameplayModifierInfo& DamageModifier = DamageEffect->Modifiers.AddDefaulted_GetRef();
	DamageModifier.Attribute = ULyraHealthSet::GetDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FScalableFloat(7.0f);
	FGameplayEffectSpec DamageSpec(DamageEffect, ASC->MakeEffectContext(), 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(DamageSpec);
	Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()), InjuredHealth - 7.0f),
		TEXT("The player can actually take damage with the reward menu open"));

	Progress->RequestCloseRewardChoices();
	Verify(!Progress->State.bRewardMenuOpen && !ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked),
		TEXT("Closing the menu restores ability input without consuming a reward"));
	if (FirstCards.IsEmpty())
	{
		return false;
	}
	Progress->RequestSelectReward(FirstRun, FirstOffer, FirstCards[0]);
	Verify(Progress->State.PendingChoices == 2 && Progress->State.ChosenRewards.IsEmpty(),
		TEXT("A selection submitted after closing the menu is rejected"));
	Progress->RequestOpenRewardChoices();
	Verify(Progress->State.OfferId == FirstOffer && CardIds(Progress) == FirstCards,
		TEXT("Closing and reopening preserves the exact offer and card order"));
	Progress->RequestSelectReward(FirstRun, FirstOffer, TEXT("ForgedReward"));
	Progress->RequestSelectReward(FirstRun - 1, FirstOffer, FirstCards[0]);
	Progress->RequestSelectReward(FirstRun, FirstOffer + 1, FirstCards[0]);
	Verify(Progress->State.PendingChoices == 2 && Progress->State.ChosenRewards.IsEmpty()
		&& Progress->State.OfferId == FirstOffer
		&& FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), BaseMaxHealth)
		&& FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), BaseDamage),
		TEXT("Forged card, wrong run and wrong offer cannot consume a chance or change attributes"));
	const FRunRewardOption ChosenCard = Progress->State.Options[0];
	Progress->RequestSelectReward(FirstRun, FirstOffer, ChosenCard.CardId);
	Verify(Progress->State.PendingChoices == 1 && Progress->State.ChosenRewards.Num() == 1
		&& FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), BaseMaxHealth + ChosenCard.GetMagnitude(ERunRewardBehavior::MaxHealthBonus))
		&& FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), BaseDamage + ChosenCard.GetMagnitude(ERunRewardBehavior::DamageBonus)),
		TEXT("Confirmed card applies its exact permanent bonuses and consumes one chance"));
	Progress->RequestSelectReward(FirstRun, FirstOffer, ChosenCard.CardId);
	Verify(Progress->State.PendingChoices == 1 && Progress->State.ChosenRewards.Num() == 1,
		TEXT("A duplicate confirmation cannot grant a second reward"));
	const int32 StaleRun = Progress->State.RunId;
	const int32 StaleOffer = Progress->State.OfferId;
	Progress->ResetRun();
	Progress->AddExperience(160);
	Progress->RequestOpenRewardChoices();
	Progress->RequestSelectReward(StaleRun, StaleOffer, ChosenCard.CardId);
	Verify(Progress->State.RunId != StaleRun && Progress->State.PendingChoices == 2 && Progress->State.ChosenRewards.IsEmpty(),
		TEXT("A prior run's outstanding request is invalid after reset"));

	// An instant heal must not become part of the recurring persistent reward reconstruction.
	Progress->ResetRun();
	Progress->Definition->RewardPool->Cards = {Card(TEXT("TestMedicine"), 0.0f, 0.0f, 0.0f, 0.4f)};
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), BaseMaxHealth * 0.2f);
	Progress->AddExperience(60);
	Progress->RequestOpenRewardChoices();
	const int32 MedicalOffer = Progress->State.OfferId;
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), BaseMaxHealth);
	Progress->RequestSelectReward(Progress->State.RunId, MedicalOffer, TEXT("TestMedicine"));
	Verify(Progress->State.PendingChoices == 1 && Progress->State.ChosenRewards.IsEmpty() && Progress->State.OfferId == MedicalOffer,
		TEXT("A heal that becomes inapplicable after drawing fails without losing the offer or chance"));
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), BaseMaxHealth * 0.2f);
	Verify(Select(Progress, TEXT("TestMedicine")), TEXT("An injured player can select the medical card"));
	const float MedicalHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
	const float ExpectedMedicalHealth = BaseMaxHealth * 0.6f;
	UE_LOG(LogTemp, Display, TEXT("MonsterArenaRewardTest: medical health actual=%.8f expected=%.8f tolerance=%.8f"), MedicalHealth, ExpectedMedicalHealth, KINDA_SMALL_NUMBER);
	Verify(FMath::IsNearlyEqual(MedicalHealth, ExpectedMedicalHealth, KINDA_SMALL_NUMBER),
		TEXT("Medical card restores forty percent of maximum health exactly once"));
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), BaseMaxHealth * 0.2f);
	Progress->TickComponent(0.2f, LEVELTICK_All, nullptr);
	Progress->TickComponent(0.2f, LEVELTICK_All, nullptr);
	Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()), BaseMaxHealth * 0.2f),
		TEXT("Periodic reward maintenance does not replay an instant heal"));

	ULyraQuickBarComponent* QuickBar = Player->GetPlayerController()->FindComponentByClass<ULyraQuickBarComponent>();
	ULyraInventoryItemInstance* Item = QuickBar ? QuickBar->GetActiveSlotItem() : nullptr;
	Verify(Item != nullptr, TEXT("Ammo reward has a real selected inventory item"));
	if (Item)
	{
		const FGameplayTag Spare = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo"));
		const FGameplayTag Magazine = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineAmmo"));
		const int32 OriginalSpare = Item->GetStatTagStackCount(Spare);
		const int32 OriginalMagazine = Item->GetStatTagStackCount(Magazine);
		Progress->ResetRun();
		Progress->Definition->RewardPool->Cards = {Card(TEXT("TestSupplies"), 0.0f, 0.0f, 0.0f, 0.0f, 60)};
		Item->RemoveStatTagStack(Spare, OriginalSpare);
		Item->AddStatTagStack(Spare, 5);
		Progress->AddExperience(60);
		Verify(Select(Progress, TEXT("TestSupplies")), TEXT("Ammo supply can be selected for the actual rifle"));
		Verify(Item->GetStatTagStackCount(Spare) == 65 && Item->GetStatTagStackCount(Magazine) == OriginalMagazine,
			TEXT("Ammo supply adds sixty reserve rounds without filling the magazine"));
		Item->RemoveStatTagStack(Spare, Item->GetStatTagStackCount(Spare));
		Item->AddStatTagStack(Spare, 7);
		Progress->TickComponent(0.2f, LEVELTICK_All, nullptr);
		Progress->TickComponent(0.2f, LEVELTICK_All, nullptr);
		Verify(Item->GetStatTagStackCount(Spare) == 7, TEXT("Periodic reward maintenance does not refill reserve ammo"));
		Item->RemoveStatTagStack(Spare, Item->GetStatTagStackCount(Spare));
		Item->AddStatTagStack(Spare, OriginalSpare);

		Progress->ResetRun();
		FRunRewardCardEntry Combined = Card(TEXT("TestCombined"));
		// Deliberately place healing before max health in source order.
		Combined.Option.Actions = {
			{ERunRewardBehavior::HealFraction, 0.25f},
			{ERunRewardBehavior::HealAmount, 5.0f},
			{ERunRewardBehavior::DamageBonus, 2.0f},
			{ERunRewardBehavior::MaxHealthBonus, 20.0f},
			{ERunRewardBehavior::DamageBonus, 3.0f},
			{ERunRewardBehavior::SpareAmmo, 12.0f}};
		Progress->Definition->RewardPool->Cards = {Combined};
		ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), 10.0f);
		Progress->AddExperience(60);
		Verify(Select(Progress, TEXT("TestCombined")), TEXT("One card grants six configured behavior records"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), BaseDamage + 5.0f)
			&& FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), BaseMaxHealth + 20.0f)
			&& FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()), 15.0f + (BaseMaxHealth + 20.0f) * 0.25f)
			&& Item->GetStatTagStackCount(Spare) == OriginalSpare + 12,
			TEXT("Repeated damage adds together and healing uses the new maximum before supplying ammo"));
		Progress->TickComponent(0.2f, LEVELTICK_All, nullptr);
		Verify(Item->GetStatTagStackCount(Spare) == OriginalSpare + 12,
			TEXT("Combined card maintenance does not repeat instant rewards"));
		Item->RemoveStatTagStack(Spare, 12);
		Combined.Option.Actions.Add({static_cast<ERunRewardBehavior>(255), 1.0f});
		Verify(!Combined.Option.IsValid(), TEXT("Unknown behaviors invalidate the whole card"));
		Combined.Option.Actions = {{ERunRewardBehavior::SpareAmmo, 16777216.0f}, {ERunRewardBehavior::SpareAmmo, 1.0f}};
		Verify(!Combined.Option.IsValid(), TEXT("Repeated ammo cannot exceed the exact integer range through rounding"));
	}

	// A limited pool is valid and must not deadlock the selection queue when its card is exhausted.
	Progress->ResetRun();
	FRunRewardCardEntry LimitedCard = Card(TEXT("TestLimited"), 0.0f, 3.0f);
	LimitedCard.MaxStacks = 1;
	Progress->Definition->RewardPool->Cards = {LimitedCard, LimitedCard, Card(TEXT("TestFullHealthMedicine"), 0.0f, 0.0f, 0.0f, 0.4f)};
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), BaseMaxHealth);
	Progress->AddExperience(160);
	Progress->RequestOpenRewardChoices();
	Verify(Progress->State.Options.Num() == 1 && Progress->State.Options[0].CardId == TEXT("TestLimited"),
		TEXT("Full-health healing and duplicate card IDs are filtered, leaving one genuine choice"));
	Verify(Select(Progress, TEXT("TestLimited")), TEXT("A capped reward can be claimed once"));
	Verify(!Progress->State.Options.IsEmpty()
		&& !Progress->State.Options.ContainsByPredicate([](const FRunRewardOption& Option) { return Option.CardId == TEXT("TestLimited"); }),
		TEXT("Capped cards leave the pool and a usable fallback keeps remaining choices claimable"));
	Progress->RequestCloseRewardChoices();
	Verify(!UGameplayStatics::IsGamePaused(Player) && !ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked),
		TEXT("Leaving the reward flow releases its input lock and never pauses gameplay"));
	ASC->AddLooseGameplayTag(TAG_Gameplay_AbilityInputBlocked);
	Progress->RequestOpenRewardChoices();
	Progress->RequestCloseRewardChoices();
	Verify(ASC->HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked),
		TEXT("Closing rewards preserves an input block owned by another system"));
	ASC->RemoveLooseGameplayTag(TAG_Gameplay_AbilityInputBlocked);
	UE_LOG(LogTemp, Display, TEXT("MONSTER_ARENA_REWARD_CHOICES_%s"), bPassed ? TEXT("SUCCEEDED") : TEXT("FAILED"));
	return bPassed;
}
#endif
