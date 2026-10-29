#pragma once

#include "Engine/DataAsset.h"
#include "RunRewardCardDefinition.generated.h"

/** A server-resolved card snapshot. The same values are displayed and granted. */
USTRUCT(BlueprintType)
struct FRunRewardOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CardId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true))
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float MaxHealthBonus = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float DamageBonus = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float HealAmount = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float HealFraction = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 SpareAmmo = 0;
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
