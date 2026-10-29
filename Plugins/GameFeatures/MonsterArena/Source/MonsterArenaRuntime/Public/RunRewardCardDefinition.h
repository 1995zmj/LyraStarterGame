#pragma once

#include "Engine/DataAsset.h"
#include "RunRewardCardDefinition.generated.h"

/** Adding a behavior requires runtime handling, but never another CSV column. */
UENUM(BlueprintType)
enum class ERunRewardBehavior : uint8
{
	MaxHealthBonus,
	DamageBonus,
	HealAmount,
	HealFraction,
	SpareAmmo,
	DamagePerKill
};

USTRUCT(BlueprintType)
struct FRunRewardAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ERunRewardBehavior Behavior = ERunRewardBehavior::DamageBonus;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Value = 0.0f;
};

/** A server-resolved card snapshot. The same actions are displayed and granted. */
USTRUCT(BlueprintType)
struct MONSTERARENARUNTIME_API FRunRewardOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CardId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true))
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Behavior"))
	TArray<FRunRewardAction> Actions;

	float GetMagnitude(ERunRewardBehavior Behavior) const;
	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct FRunRewardCardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRunRewardOption Option;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Weight = 1.0f;
	/** Zero means that this card can be selected any number of times this run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 MaxStacks = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "2"))
	int32 MinimumLevel = 2;
};

/** Optional designer-authored pool; native defaults keep the feature self-contained. */
UCLASS(BlueprintType)
class MONSTERARENARUNTIME_API URunRewardPoolDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	URunRewardPoolDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
	TArray<FRunRewardCardEntry> Cards;
};
