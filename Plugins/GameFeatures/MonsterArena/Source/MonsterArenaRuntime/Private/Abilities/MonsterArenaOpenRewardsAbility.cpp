#include "Abilities/MonsterArenaOpenRewardsAbility.h"

#include "GameFramework/GameStateBase.h"
#include "MonsterArenaComponent.h"
#include "MonsterArenaGameplayTags.h"
#include "Player/LyraPlayerController.h"
#include "UI/MonsterArenaHUD.h"

UMonsterArenaOpenRewardsAbility::UMonsterArenaOpenRewardsAbility()
{
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_MonsterArena_Ability_OpenRewards);
	SetAssetTags(Tags);
}

void UMonsterArenaOpenRewardsAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (ALyraPlayerController* PC = GetLyraPlayerControllerFromActorInfo())
	{
		if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
		{
			if (UMonsterArenaComponent* Arena = GameState->FindComponentByClass<UMonsterArenaComponent>())
			{
				if (UMonsterArenaHUD* HUD = Arena->FindHUDForPlayer(PC))
				{
					HUD->OpenRewardMenu();
				}
			}
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
