#include "MonsterArenaContentLibrary.h"
#include "GameFeatureAction_AddComponents.h"
#include "GameFeatures/GameFeatureAction_AddAbilities.h"
#include "GameModes/LyraExperienceDefinition.h"
#include "GameModes/LyraExperienceActionSet.h"
#include "Character/LyraPawnData.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/UnrealType.h"
#include "GameFeatureData.h"
#include "RunProgressionComponent.h"

bool UMonsterArenaContentLibrary::BindProgression(ULyraExperienceDefinition* Experience, TSubclassOf<UActorComponent> ComponentClass)
{
	if (!Experience || !ComponentClass || !ComponentClass->IsChildOf(URunProgressionComponent::StaticClass())) { return false; }
	FGameFeatureComponentEntry* Match = nullptr;
	UGameFeatureAction_AddComponents* Owner = nullptr;
	for (UGameFeatureAction* Action : Experience->Actions)
	{
		if (auto* Components = Cast<UGameFeatureAction_AddComponents>(Action))
		{
			for (auto& Entry : Components->ComponentList)
			{
				UClass* Existing = Entry.ComponentClass.LoadSynchronous();
				if (Existing && Existing->IsChildOf(URunProgressionComponent::StaticClass()))
				{
					if (Match) { return false; }
					Match = &Entry;
					Owner = Components;
				}
			}
		}
	}
	if (!Match) { return false; }
	Experience->Modify();
	Owner->Modify();
	Match->ComponentClass = ComponentClass.Get();
	return true;
}

void UMonsterArenaContentLibrary::ConfigureWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}
	AWorldSettings* Settings = World->GetWorldSettings();
	FSoftClassProperty* Property = FindFProperty<FSoftClassProperty>(Settings->GetClass(), TEXT("DefaultGameplayExperience"));
	if (ensure(Property))
	{
		Settings->Modify();
		*Property->ContainerPtrToValuePtr<TSoftClassPtr<ULyraExperienceDefinition>>(Settings) = TSoftClassPtr<ULyraExperienceDefinition>(FSoftObjectPath(TEXT("/MonsterArena/System/Experiences/B_MonsterArena.B_MonsterArena_C")));
	}
}

void UMonsterArenaContentLibrary::ConfigureExperience(ULyraExperienceDefinition* Experience)
{
	if (!Experience)
	{
		return;
	}
	Experience->Modify();
	Experience->GameFeaturesToEnable = {TEXT("ShooterCore"), TEXT("MonsterArena")};
	Experience->DefaultPawnData = LoadObject<ULyraPawnData>(nullptr, TEXT("/MonsterArena/System/HeroData_MonsterArena.HeroData_MonsterArena"));
	Experience->ActionSets.Reset();
	for (const TCHAR* Path : {
			 TEXT("/ShooterCore/Experiences/LAS_ShooterGame_SharedInput.LAS_ShooterGame_SharedInput"),
			 TEXT("/MonsterArena/System/Experiences/LAS_MonsterArena_Components.LAS_MonsterArena_Components"),
			 TEXT("/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD.LAS_ShooterGame_StandardHUD")})
	{
		Experience->ActionSets.Add(LoadObject<ULyraExperienceActionSet>(nullptr, Path));
	}
	UGameFeatureAction_AddComponents* Components = NewObject<UGameFeatureAction_AddComponents>(Experience);
	auto Add = [Components](const TCHAR* Actor, const TCHAR* Component, bool bClient) {
		FGameFeatureComponentEntry Entry;
		Entry.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(Actor));
		Entry.ComponentClass = TSoftClassPtr<UActorComponent>(FSoftObjectPath(Component));
		Entry.bClientComponent = bClient;
		Entry.bServerComponent = true;
		Components->ComponentList.Add(Entry);
	};
	Add(TEXT("/Script/LyraGame.LyraGameState"), TEXT("/Script/MonsterArenaRuntime.MonsterArenaComponent"), true);
	Add(TEXT("/Script/LyraGame.LyraPlayerState"), TEXT("/Script/MonsterArenaRuntime.RunProgressionComponent"), true);
	Add(TEXT("/Script/LyraGame.LyraGameState"), TEXT("/ShooterCore/Game/B_TeamSetup_TwoTeams.B_TeamSetup_TwoTeams_C"), false);
	Add(TEXT("/Script/LyraGame.LyraGameState"), TEXT("/ShooterCore/Game/B_TeamSpawningRules.B_TeamSpawningRules_C"), false);
	Add(TEXT("/Script/Engine.Controller"), TEXT("/Game/Characters/Cosmetics/B_PickRandomCharacter.B_PickRandomCharacter_C"), false);
	UGameFeatureAction_AddAbilities* Abilities = NewObject<UGameFeatureAction_AddAbilities>(Experience);
	FGameFeatureAbilitiesEntry GrantEntry;
	GrantEntry.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Script/LyraGame.LyraPlayerState")));
	FLyraAbilityGrant Grant;
	Grant.AbilityType = TSoftClassPtr<UGameplayAbility>(FSoftObjectPath(TEXT("/ShooterCore/Game/Respawn/GA_AutoRespawn.GA_AutoRespawn_C")));
	GrantEntry.GrantedAbilities.Add(Grant);
	Abilities->AbilitiesList.Add(GrantEntry);
	Experience->Actions = {Components, Abilities};
}

void UMonsterArenaContentLibrary::ConfigureEditorAssetScope()
{
	UGameFeatureData* SharedFeature = LoadObject<UGameFeatureData>(nullptr, TEXT("/ShooterCore/ShooterCore.ShooterCore"));
	ULyraExperienceActionSet* SharedSet = LoadObject<ULyraExperienceActionSet>(nullptr, TEXT("/ShooterCore/Experiences/LAS_ShooterGame_StandardComponents.LAS_ShooterGame_StandardComponents"));
	ULyraExperienceActionSet* LocalSet = LoadObject<ULyraExperienceActionSet>(nullptr, TEXT("/MonsterArena/System/Experiences/LAS_MonsterArena_Components.LAS_MonsterArena_Components"));
	if (!ensure(SharedFeature && SharedSet && LocalSet))
	{
		return;
	}
	const FSoftObjectPath SharedRelay(TEXT("/ShooterCore/Accolades/B_EliminationFeedRelay.B_EliminationFeedRelay_C"));
	const FSoftObjectPath LocalRelay(TEXT("/MonsterArena/UI/Notifications/B_MonsterArenaEliminationFeedRelay.B_MonsterArenaEliminationFeedRelay_C"));
	SharedFeature->Modify();
	// Relay selection belongs to the experience, rather than every consumer of the gun plugin.
	for (UGameFeatureAction* Action : SharedFeature->GetActions())
	{
		if (UGameFeatureAction_AddComponents* Components = Cast<UGameFeatureAction_AddComponents>(Action))
		{
			Components->ComponentList.RemoveAll([&SharedRelay](const FGameFeatureComponentEntry& Entry) { return Entry.ComponentClass.ToSoftObjectPath() == SharedRelay; });
		}
	}
	auto ConfigureRelay = [](ULyraExperienceActionSet* Set, const FSoftObjectPath& Relay, bool bLocal) {
		Set->Modify();
		UGameFeatureAction_AddComponents* Components = nullptr;
		for (UGameFeatureAction* Action : Set->Actions)
		{
			Components = Cast<UGameFeatureAction_AddComponents>(Action);
			if (Components)
			{
				break;
			}
		}
		if (!Components)
		{
			Components = NewObject<UGameFeatureAction_AddComponents>(Set);
			Set->Actions.Add(Components);
		}
		Components->ComponentList.RemoveAll([](const FGameFeatureComponentEntry& Entry) {
			const FString Path = Entry.ComponentClass.ToSoftObjectPath().ToString();
			return Path == TEXT("/ShooterCore/Accolades/B_EliminationFeedRelay.B_EliminationFeedRelay_C") || Path == TEXT("/MonsterArena/UI/Notifications/B_MonsterArenaEliminationFeedRelay.B_MonsterArenaEliminationFeedRelay_C");
		});
		if (bLocal)
		{
			for (FGameFeatureComponentEntry& Entry : Components->ComponentList)
			{
				if (Entry.ActorClass.ToSoftObjectPath().ToString() == TEXT("/ShooterCore/Game/B_Hero_ShooterMannequin.B_Hero_ShooterMannequin_C") || Entry.ActorClass.ToSoftObjectPath().ToString() == TEXT("/MonsterArena/Characters/B_MonsterArenaHero.B_MonsterArenaHero_C"))
				{
					Entry.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/MonsterArena/Characters/B_MonsterArenaShooterHero.B_MonsterArenaShooterHero_C")));
				}
			}
		}
		FGameFeatureComponentEntry Entry;
		Entry.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Script/Engine.GameStateBase")));
		Entry.ComponentClass = TSoftClassPtr<UActorComponent>(Relay);
		Entry.bClientComponent = true;
		Entry.bServerComponent = true;
		Components->ComponentList.Add(Entry);
	};
	ConfigureRelay(SharedSet, SharedRelay, false);
	ConfigureRelay(LocalSet, LocalRelay, true);
}
