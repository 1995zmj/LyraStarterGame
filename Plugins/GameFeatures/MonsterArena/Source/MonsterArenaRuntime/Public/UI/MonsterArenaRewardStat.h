#pragma once

#include "Blueprint/UserWidget.h"
#include "RunRewardCardDefinition.h"
#include "MonsterArenaRewardStat.generated.h"

class UTextBlock;

/** One designer-authored effect row, configured by its owning card. */
UCLASS(Abstract, Blueprintable)
class MONSTERARENARUNTIME_API UMonsterArenaRewardStat : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void Configure(ERunRewardBehavior InBehavior, float InValue, const FText& Preview);
protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|Rewards")
	ERunRewardBehavior Behavior = ERunRewardBehavior::DamageBonus;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|Rewards")
	float Value = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> EffectName;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> EffectValue;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> EffectPreview;
};
