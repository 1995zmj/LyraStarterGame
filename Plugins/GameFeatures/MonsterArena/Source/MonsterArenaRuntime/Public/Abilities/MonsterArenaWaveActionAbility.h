#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MonsterArenaWaveActionAbility.generated.h"

/**
 * Input-driven ability granted by AbilitySet_MonsterArena. Runs on the owning
 * client and forwards the wave request through RunProgressionComponent, whose
 * server RPC keeps the existing RunId/WaveIndex validation.
 */
UCLASS()
class MONSTERARENARUNTIME_API UMonsterArenaWaveActionAbility : public ULyraGameplayAbility
{
	GENERATED_BODY()
public:
	UMonsterArenaWaveActionAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
