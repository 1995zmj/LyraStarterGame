#pragma once

#include "GameplayEffect.h"
#include "RunRewardEffects.generated.h"

UCLASS()
class MONSTERARENARUNTIME_API URunHealthRewardEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	URunHealthRewardEffect();
};

UCLASS()
class MONSTERARENARUNTIME_API URunDamageRewardEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	URunDamageRewardEffect();
};
