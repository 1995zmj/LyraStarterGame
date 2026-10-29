#include "ArenaMonster.h"
#include "MonsterArenaRewardTestHelpers.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Player/LyraPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FMonsterArenaLifecycleState
{
	TWeakObjectPtr<ALyraPlayerState> Player;
	TWeakObjectPtr<URunProgressionComponent> Progress;
	TWeakObjectPtr<APawn> PreviousPawn;
	TWeakObjectPtr<AArenaMonster> Killer;
	TStrongObjectPtr<URunProgressionDefinition> OriginalDefinition;
	TArray<FName> PendingCards;
	int32 PendingOffer = 0;
	int32 Deaths = 0;
	int32 InitialSpareAmmo = 0;
	float ExpectedMaxHealth = 0.0f;
	float ExpectedDamage = 0.0f;
	double StartedAt = 0.0;
	double StepStartedAt = 0.0;
	double NormalRespawnReadyAt = -1.0;
	bool bOriginalTickEnabled = true;
	bool bInitialized = false;
	bool bWaitingForRespawn = false;
	bool bPassed = true;
	bool bFinished = false;
};
}

void TickMonsterArenaLifecycleTest(ALyraPlayerState* Player, float DeltaTime)
{
	using namespace MonsterArenaRewardTests;
	static FMonsterArenaLifecycleState Test;
	URunProgressionComponent* Progress = Player ? Player->FindComponentByClass<URunProgressionComponent>() : nullptr;
	if (!Progress)
	{
		return;
	}
	if (Test.Player.Get() != Player)
	{
		if (URunProgressionComponent* PreviousProgress = Test.Progress.Get())
		{
			PreviousProgress->SetComponentTickEnabled(Test.bOriginalTickEnabled);
			PreviousProgress->Definition = Test.OriginalDefinition.Get();
		}
		Test = FMonsterArenaLifecycleState();
		Test.Player = Player;
		Test.Progress = Progress;
		Test.OriginalDefinition.Reset(Progress->Definition.Get());
		Test.bOriginalTickEnabled = Progress->IsComponentTickEnabled();
		Test.StartedAt = Test.StepStartedAt = FPlatformTime::Seconds();
	}
	if (Test.bFinished)
	{
		return;
	}
	auto Verify = [](bool bCondition, const TCHAR* Description)
	{
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaLifecycleTest: %s: %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), Description);
		Test.bPassed &= bCondition;
	};
	auto Finish = [Progress]()
	{
		if (Test.Killer.IsValid())
		{
			Test.Killer->Destroy();
		}
		Progress->SetComponentTickEnabled(Test.bOriginalTickEnabled);
		Progress->RequestCloseRewardChoices();
		Progress->ResetRun();
		Progress->Definition = Test.OriginalDefinition.Get();
		Test.OriginalDefinition.Reset();
		Test.bFinished = true;
		UE_LOG(LogTemp, Display, TEXT("MONSTER_ARENA_LIFECYCLE_%s"), Test.bPassed ? TEXT("SUCCEEDED") : TEXT("FAILED"));
		FPlatformMisc::RequestExitWithStatus(false, Test.bPassed ? 0 : 1);
	};
	if (FPlatformTime::Seconds() - Test.StartedAt > 90.0)
	{
		Verify(false, TEXT("Lifecycle test completed within 90 seconds"));
		Finish();
		return;
	}
	APawn* Pawn = Player->GetPawn();
	ULyraAbilitySystemComponent* ASC = Player->GetLyraAbilitySystemComponent();
	APlayerController* Controller = Player->GetPlayerController();
	ULyraQuickBarComponent* QuickBar = Controller ? Controller->FindComponentByClass<ULyraQuickBarComponent>() : nullptr;
	ULyraInventoryItemInstance* Rifle = QuickBar ? QuickBar->GetActiveSlotItem() : nullptr;
	if (!Pawn || !ASC || !Rifle || ASC->GetAvatarActor() != Pawn)
	{
		return;
	}
	const FGameplayTag Spare = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo"));
	if (!Test.bInitialized)
	{
		if (FPlatformTime::Seconds() - Test.StepStartedAt < 2.0)
		{
			return;
		}
		Progress->ResetRun();
		Test.InitialSpareAmmo = Rifle->GetStatTagStackCount(Spare);
		Test.ExpectedMaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()) + 20.0f;
		Test.ExpectedDamage = ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()) + 3.0f;
		const URunProgressionDefinition* Source = Progress->Definition ? Progress->Definition.Get() : GetDefault<URunProgressionDefinition>();
		Progress->Definition = DuplicateObject<URunProgressionDefinition>(Source, Progress);
		Progress->Definition->RewardPool = NewObject<URunRewardPoolDefinition>(Progress->Definition);
		Progress->Definition->RewardPool->Cards = {
			Card(TEXT("LifecycleVitality"), 20.0f, 0.0f, 20.0f),
			Card(TEXT("LifecycleFirepower"), 0.0f, 3.0f),
			Card(TEXT("LifecycleSupplies"), 0.0f, 0.0f, 10.0f, 0.0f, 60)};
		Progress->AddExperience(480);
		Verify(Progress->State.Level == 5 && Progress->State.PendingChoices == 4, TEXT("Four upgrades queue four unclaimed rewards"));
		Verify(Select(Progress, TEXT("LifecycleVitality")), TEXT("Vitality is explicitly chosen before respawn"));
		Verify(Select(Progress, TEXT("LifecycleFirepower")), TEXT("Firepower is explicitly chosen before respawn"));
		ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), 40.0f);
		Rifle->RemoveStatTagStack(Spare, Rifle->GetStatTagStackCount(Spare));
		Verify(Select(Progress, TEXT("LifecycleSupplies")), TEXT("Instant healing and reserve ammunition are explicitly chosen"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()), 50.0f)
			&& Rifle->GetStatTagStackCount(Spare) == 60, TEXT("Instant supplies are granted once at selection"));
		Test.PendingOffer = Progress->State.OfferId;
		Test.PendingCards = CardIds(Progress);
		Progress->RequestCloseRewardChoices();
		Verify(Progress->State.PendingChoices == 1 && !Test.PendingCards.IsEmpty(), TEXT("One unfinished offer remains before death"));
		Test.bInitialized = true;
	}
	if (Test.bWaitingForRespawn)
	{
		if (Pawn == Test.PreviousPawn.Get())
		{
			return;
		}
		if (Test.Killer.IsValid())
		{
			Test.Killer->Destroy();
		}
		if (Test.Deaths == 0)
		{
			// Hold only reward maintenance until this point so the real new avatar's first
			// reconstruction is observable; death, respawn, inventory and GAS all run normally.
			ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), 20.0f);
			Progress->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
			Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()), 20.0f),
				TEXT("First reconstruction on the actual new pawn does not replay either instant heal"));
			Progress->SetComponentTickEnabled(Test.bOriginalTickEnabled);
		}
		else
		{
			// Let normal reward maintenance run after the equipped pawn is observed, so
			// checking before its first tick cannot hide a max-health removal/reapply clamp.
			const double WorldTime = Player->GetWorld()->GetTimeSeconds();
			if (Test.NormalRespawnReadyAt < 0.0)
			{
				Test.NormalRespawnReadyAt = WorldTime;
				return;
			}
			if (WorldTime - Test.NormalRespawnReadyAt < 0.5)
			{
				return;
			}
			const float RespawnHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
			UE_LOG(LogTemp, Display, TEXT("MonsterArenaLifecycleTest: normal respawn health %.2f, expected %.2f"), RespawnHealth, Test.ExpectedMaxHealth);
			Verify(FMath::IsNearlyEqual(RespawnHealth, Test.ExpectedMaxHealth),
				TEXT("Second normal respawn remains at the full rewarded maximum health after maintenance"));
		}
		Verify(Progress->State.Level == 5 && Progress->State.PendingChoices == 1 && Progress->State.ChosenRewards.Num() == 3,
			TEXT("Level, unclaimed choice and selected reward ledger survive death"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), Test.ExpectedMaxHealth),
			TEXT("Selected maximum health is restored once on the new pawn"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), Test.ExpectedDamage),
			TEXT("Selected damage is restored without duplicate stacking"));
		Verify(Rifle->GetStatTagStackCount(Spare) == Test.InitialSpareAmmo,
			TEXT("New rifle receives its normal initial ammo without replaying the supply card"));
		Verify(!Progress->State.bRewardMenuOpen && Progress->State.OfferId == Test.PendingOffer && CardIds(Progress) == Test.PendingCards,
			TEXT("Respawn leaves the same unfinished offer closed until manually reopened"));
		Progress->RequestOpenRewardChoices();
		Verify(Progress->State.bRewardMenuOpen && Progress->State.OfferId == Test.PendingOffer && CardIds(Progress) == Test.PendingCards,
			TEXT("Manual reopening after respawn retains the original candidates"));
		Progress->RequestCloseRewardChoices();
		++Test.Deaths;
		Test.bWaitingForRespawn = false;
		Test.StepStartedAt = FPlatformTime::Seconds();
		if (Test.Deaths == 2)
		{
			Finish();
		}
		return;
	}
	if (FPlatformTime::Seconds() - Test.StepStartedAt < 2.0 || Test.Deaths >= 2)
	{
		return;
	}
	if (Test.Deaths == 0)
	{
		Progress->SetComponentTickEnabled(false);
	}
	Test.PreviousPawn = Pawn;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Test.Killer = Player->GetWorld()->SpawnActor<AArenaMonster>(AArenaMonster::StaticClass(), Pawn->GetActorLocation() + FVector(100, 0, 0), FRotator::ZeroRotator, Params);
	Verify(Test.Killer.IsValid(), TEXT("Real killer monster spawned"));
	if (Test.Killer.IsValid())
	{
		Test.Killer->MeleeDamage = 1000.0f;
		Test.bWaitingForRespawn = true;
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaLifecycleTest: waiting for monster kill %d"), Test.Deaths + 1);
	}
	else
	{
		Finish();
	}
}
#endif
