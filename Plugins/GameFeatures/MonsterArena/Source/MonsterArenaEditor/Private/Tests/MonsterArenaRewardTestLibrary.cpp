#include "MonsterArenaRewardTestLibrary.h"

#include "ArenaMeleeDamageEffect.h"
#include "ArenaMonster.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "Messages/LyraVerbMessage.h"
#include "Teams/LyraTeamSubsystem.h"
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

// Test-only damage uses normal GAS health/death callbacks and source attribution.
bool UMonsterArenaRewardTestLibrary::DealAttributedDamage(APawn* Source, APawn* Target, float Amount)
{
	ALyraPlayerState* Player = Source ? Source->GetPlayerState<ALyraPlayerState>() : nullptr;
	ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!ASC || !TargetASC || !Source->HasAuthority() || !Target->HasAuthority() || Amount <= 0)
	{
		return false;
	}
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UArenaMeleeDamageEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
	Spec.Data->SetSetByCallerMagnitude(TAG_MonsterArena_MeleeDamage, Amount);
	ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	return true;
}

bool UMonsterArenaRewardTestLibrary::KillTestMonster(APawn* Source)
{
	if (!Source || !Source->HasAuthority()) { return false; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaMonster* Monster = Source->GetWorld()->SpawnActor<AArenaMonster>(AArenaMonster::StaticClass(), Source->GetActorLocation() + FVector(500, 0, 0), FRotator::ZeroRotator, Params);
	if (!Monster) { return false; }
	Monster->SetActorTickEnabled(false);
	const bool bApplied = DealAttributedDamage(Source, Monster, 10000.0f);
	URunProgressionComponent* Progress = Source->GetPlayerState()->FindComponentByClass<URunProgressionComponent>();
	const int32 Kills = Progress->State.Kills;
	// Duplicate settlement must not increment XP, kill count or accumulated damage.
	const float Accumulated = Progress->State.AccumulatedKillDamage;
	Progress->RecordMonsterKill(Monster, Monster->ExperienceReward);
	const bool bOnce = Progress->State.Kills == Kills && Progress->State.AccumulatedKillDamage == Accumulated;
	Monster->Destroy();
	return bApplied && bOnce;
}

bool UMonsterArenaRewardTestLibrary::UseAccumulationPool(URunProgressionComponent* Progression)
{
	if (!Progression || !Progression->GetOwner()->HasAuthority() || !Progression->Definition || !Progression->Definition->RewardPool) { return false; }
	const FRunRewardCardEntry* Entry = Progression->Definition->RewardPool->Cards.FindByPredicate([](const FRunRewardCardEntry& Card) { return Card.Option.CardId == TEXT("FirepowerAccumulation"); });
	if (!Entry) { return false; }
	const FRunRewardCardEntry Copy = *Entry;
	Progression->Definition = DuplicateObject<URunProgressionDefinition>(Progression->Definition, Progression);
	Progression->Definition->RewardPool = NewObject<URunRewardPoolDefinition>(Progression->Definition);
	Progression->Definition->RewardPool->Cards = {Copy};
	return true;
}

bool UMonsterArenaRewardTestLibrary::SetTestTeam(APawn* Pawn, int32 Team)
{
	if (!Pawn || !Pawn->HasAuthority())
	{
		return false;
	}
	ULyraTeamSubsystem* Teams = Pawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
	return Teams && Teams->DoesTeamExist(Team) && Teams->ChangeTeamForActor(Pawn->GetPlayerState(), Team);
}

void UMonsterArenaRewardTestLibrary::RepeatElimination(APawn* Source, APawn* Target)
{
	if (!Source || !Target || !Source->HasAuthority()) { return; }
	FLyraVerbMessage Message;
	Message.Verb = FGameplayTag::RequestGameplayTag(TEXT("Lyra.Elimination.Message"));
	Message.Instigator = Source->GetPlayerState();
	Message.Target = Target->GetPlayerState();
	UGameplayMessageSubsystem::Get(Source).BroadcastMessage(Message.Verb, Message);
}

float UMonsterArenaRewardTestLibrary::GetBaseDamage(APawn* Pawn)
{
	const ALyraPlayerState* Player = Pawn ? Pawn->GetPlayerState<ALyraPlayerState>() : nullptr;
	const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	return ASC ? ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()) : -1.0f;
}
