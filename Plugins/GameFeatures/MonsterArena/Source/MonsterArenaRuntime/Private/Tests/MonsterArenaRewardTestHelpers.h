#pragma once

#include "RunProgressionComponent.h"
#include "RunRewardCardDefinition.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace MonsterArenaRewardTests
{
inline FRunRewardCardEntry Card(FName Id, float MaxHealth = 0.0f, float Damage = 0.0f, float Heal = 0.0f, float HealFraction = 0.0f, int32 SpareAmmo = 0)
{
	FRunRewardCardEntry Entry;
	Entry.Option.CardId = Id;
	Entry.Option.Title = FText::FromName(Id);
	Entry.Option.MaxHealthBonus = MaxHealth;
	Entry.Option.DamageBonus = Damage;
	Entry.Option.HealAmount = Heal;
	Entry.Option.HealFraction = HealFraction;
	Entry.Option.SpareAmmo = SpareAmmo;
	return Entry;
}

/** Use real reward application with a controlled pool; never edit saved configuration assets. */
class FScopedPool
{
public:
	explicit FScopedPool(URunProgressionComponent* InProgress, const TArray<FRunRewardCardEntry>& Cards)
		: Progress(InProgress), OriginalDefinition(InProgress->Definition.Get())
	{
		const URunProgressionDefinition* Source = InProgress->Definition ? InProgress->Definition.Get() : GetDefault<URunProgressionDefinition>();
		InProgress->Definition = DuplicateObject<URunProgressionDefinition>(Source, InProgress);
		InProgress->Definition->RewardPool = NewObject<URunRewardPoolDefinition>(InProgress->Definition);
		InProgress->Definition->RewardPool->Cards = Cards;
	}

	~FScopedPool()
	{
		Progress->RequestCloseRewardChoices();
		Progress->ResetRun();
		Progress->Definition = OriginalDefinition.Get();
	}

private:
	URunProgressionComponent* Progress;
	TStrongObjectPtr<URunProgressionDefinition> OriginalDefinition;
};

inline bool Select(URunProgressionComponent* Progress, FName Id)
{
	if (!Progress->State.bRewardMenuOpen)
	{
		Progress->RequestOpenRewardChoices();
	}
	if (!Progress->State.Options.ContainsByPredicate([Id](const FRunRewardOption& Option) { return Option.CardId == Id; }))
	{
		return false;
	}
	const int32 PreviousGrants = Progress->State.ChosenRewards.Num();
	Progress->RequestSelectReward(Progress->State.RunId, Progress->State.OfferId, Id);
	return Progress->State.ChosenRewards.Num() == PreviousGrants + 1;
}

inline TArray<FName> CardIds(const URunProgressionComponent* Progress)
{
	TArray<FName> Result;
	for (const FRunRewardOption& Option : Progress->State.Options)
	{
		Result.Add(Option.CardId);
	}
	return Result;
}
}
#endif
