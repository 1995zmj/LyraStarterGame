#include "MonsterArenaRewardTestLibrary.h"

#include "ArenaMeleeDamageEffect.h"
#include "CoreGlobals.h"
#include "Editor.h"
#include "Engine/World.h"
#include "MonsterArenaComponent.h"
#include "MonsterArenaRewardScreen.h"
#include "PlayInEditorDataTypes.h"
#include "RunProgressionComponent.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Templates/UnrealTemplate.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Player/LyraPlayerState.h"

bool UMonsterArenaRewardTestLibrary::StartSession(int32 PlayerCount)
{
	if (!GEditor || GEditor->PlayWorld || GEditor->GetPlaySessionRequest().IsSet())
	{
		return false;
	}
	ULevelEditorPlaySettings* Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
	Settings->SetPlayNumberOfClients(FMath::Clamp(PlayerCount, 1, 2));
	Settings->SetPlayNetMode(PlayerCount > 1 ? EPlayNetMode::PIE_ListenServer : EPlayNetMode::PIE_Standalone);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetClientWindowSize(FIntPoint(1280, 720));
	FRequestPlaySessionParams Params;
	Params.EditorPlaySettings = Settings;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	Params.SessionDestination = EPlaySessionDestinationType::InProcess;
	Params.GlobalMapOverride = TEXT("/MonsterArena/Maps/L_MonsterArena");
	Params.bAllowOnlineSubsystem = false;
	GEditor->RequestPlaySession(Params);
	return true;
}

TArray<UWorld*> UMonsterArenaRewardTestLibrary::GetTestWorlds()
{
	TArray<UWorld*> Worlds;
	if (GEditor)
	{
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				Worlds.Add(Context.World());
			}
		}
	}
	return Worlds;
}

void UMonsterArenaRewardTestLibrary::StopSession()
{
	if (GEditor && GEditor->PlayWorld)
	{
		GEditor->RequestEndPlayMap();
	}
}

bool UMonsterArenaRewardTestLibrary::DealDamage(APawn* Pawn, float Amount)
{
	ALyraPlayerState* Player = IsValid(Pawn) ? Pawn->GetPlayerState<ALyraPlayerState>() : nullptr;
	ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	if (!IsValid(Pawn) || !Pawn->HasAuthority() || !IsValid(ASC) || Amount <= 0.0f)
	{
		return false;
	}
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UArenaMeleeDamageEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return false;
	}
	Spec.Data->SetSetByCallerMagnitude(TAG_MonsterArena_MeleeDamage, Amount);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return true;
}

bool UMonsterArenaRewardTestLibrary::OpenRewardMenu(UMonsterArenaHUD* HUD)
{
	if (!IsValid(HUD))
	{
		return false;
	}
	// Python's reflection guard otherwise makes AActor::GetFunctionCallspace
	// return Local even for a Server RPC on the owning PIE client.
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	HUD->OpenRewardMenu();
	return true;
}

bool UMonsterArenaRewardTestLibrary::CloseRewardMenu(UMonsterArenaHUD* HUD)
{
	if (!IsValid(HUD))
	{
		return false;
	}
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	HUD->CloseRewardMenu();
	return true;
}

bool UMonsterArenaRewardTestLibrary::SelectAndConfirmReward(UMonsterArenaRewardScreen* Screen, int32 Index)
{
	if (!IsValid(Screen))
	{
		return false;
	}
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	Screen->SelectCard(Index);
	Screen->ConfirmSelection();
	return true;
}

bool UMonsterArenaRewardTestLibrary::RequestCard(URunProgressionComponent* Progression, int32 RunId, int32 OfferId, FName CardId)
{
	if (!IsValid(Progression))
	{
		return false;
	}
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	Progression->RequestSelectReward(RunId, OfferId, CardId);
	return true;
}

int32 UMonsterArenaRewardTestLibrary::GetInputBlockCount(APawn* Pawn)
{
	const ALyraPlayerState* Player = Pawn ? Pawn->GetPlayerState<ALyraPlayerState>() : nullptr;
	const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	return ASC ? ASC->GetTagCount(TAG_Gameplay_AbilityInputBlocked) : -1;
}

bool UMonsterArenaRewardTestLibrary::IsDamageImmune(APawn* Pawn)
{
	const ALyraPlayerState* Player = Pawn ? Pawn->GetPlayerState<ALyraPlayerState>() : nullptr;
	const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	return !ASC || ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Gameplay.DamageImmunity")));
}
