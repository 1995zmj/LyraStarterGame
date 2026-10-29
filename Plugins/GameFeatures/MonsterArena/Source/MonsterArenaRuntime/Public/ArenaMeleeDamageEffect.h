#pragma once

#include "GameplayEffect.h"
#include "NativeGameplayTags.h"
#include "ArenaMeleeDamageEffect.generated.h"

MONSTERARENARUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MonsterArena_MeleeDamage);

/** Stable effect definition for melee damage replicated through Lyra's damage/death events. */
UCLASS()
class MONSTERARENARUNTIME_API UArenaMeleeDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UArenaMeleeDamageEffect();
};
