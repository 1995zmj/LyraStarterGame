#include "Abilities/MonsterArenaWaveActionAbility.h"

#include "ArenaWaveDirectorComponent.h"
#include "GameFramework/GameStateBase.h"
#include "MonsterArenaGameplayTags.h"
#include "Player/LyraPlayerController.h"
#include "Player/LyraPlayerState.h"
#include "RunProgressionComponent.h"

UMonsterArenaWaveActionAbility::UMonsterArenaWaveActionAbility()
{
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_MonsterArena_Ability_WaveAction);
	SetAssetTags(Tags);
}

void UMonsterArenaWaveActionAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (ALyraPlayerController* PC = GetLyraPlayerControllerFromActorInfo())
	{
		ALyraPlayerState* Player = PC->GetPlayerState<ALyraPlayerState>();
		URunProgressionComponent* Progression = Player ? Player->FindComponentByClass<URunProgressionComponent>() : nullptr;
		const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
		const UArenaWaveDirectorComponent* Director = GameState ? GameState->FindComponentByClass<UArenaWaveDirectorComponent>() : nullptr;
		if (Progression && Director && !Progression->State.bRewardMenuOpen)
		{
			Progression->RequestWaveAction(Director->State.RunId, Director->State.WaveIndex);
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
