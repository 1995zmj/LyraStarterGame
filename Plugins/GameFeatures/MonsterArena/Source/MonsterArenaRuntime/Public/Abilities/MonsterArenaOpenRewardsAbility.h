#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MonsterArenaOpenRewardsAbility.generated.h"

/**
 * Input-driven ability granted by AbilitySet_MonsterArena. It only runs on the
 * owning client and forwards the request to the local HUD, which keeps the
 * CommonUI modal flow, input blocking and server validation unchanged.
 */
UCLASS()
class MONSTERARENARUNTIME_API UMonsterArenaOpenRewardsAbility : public ULyraGameplayAbility
{
	GENERATED_BODY()
public:
	UMonsterArenaOpenRewardsAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
