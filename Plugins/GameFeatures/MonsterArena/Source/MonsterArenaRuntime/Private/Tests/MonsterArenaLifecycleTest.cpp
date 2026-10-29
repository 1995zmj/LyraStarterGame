#include "ArenaMonster.h"
#include "RunProgressionComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "Character/LyraHealthComponent.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "Player/LyraPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
void TickMonsterArenaLifecycleTest(ALyraPlayerState* Player, float DeltaTime)
{
	static TWeakObjectPtr<APawn> PreviousPawn;
	static TWeakObjectPtr<AArenaMonster> Killer;
	static int32 Deaths = 0;
	static float Elapsed = 0.0f;
	static bool bWaitingForRespawn = false;
	static bool bPassed = true;
	Elapsed += DeltaTime;
	auto Verify = [](bool bCondition, const TCHAR* Description) {
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaLifecycleTest: %s: %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), Description);
		bPassed &= bCondition;
	};
	APawn* Pawn = Player->GetPawn();
	ULyraAbilitySystemComponent* ASC = Player->GetLyraAbilitySystemComponent();
	URunProgressionComponent* Progress = Player->FindComponentByClass<URunProgressionComponent>();
	if (!Pawn || !ASC || !Progress)
	{
		return;
	}
	if (bWaitingForRespawn)
	{
		if (Pawn == PreviousPawn.Get())
		{
			return;
		}
		if (Killer.IsValid())
		{
			Killer->Destroy();
		}
		Verify(Progress->State.Level == 3, TEXT("Run level survives monster kill and respawn"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), 120.0f), TEXT("Health reward restored on new pawn"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), 3.0f), TEXT("Damage reward restored without stacking duplicates"));
		ULyraQuickBarComponent* QuickBar = Player->GetPlayerController()->FindComponentByClass<ULyraQuickBarComponent>();
		Verify(QuickBar && QuickBar->GetActiveSlotItem(), TEXT("Respawn has a valid selected weapon item"));
		++Deaths;
		bWaitingForRespawn = false;
		Elapsed = 0.0f;
		if (Deaths == 2)
		{
			UE_LOG(LogTemp, Display, TEXT("MONSTER_ARENA_LIFECYCLE_%s"), bPassed ? TEXT("SUCCEEDED") : TEXT("FAILED"));
			FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
		}
		return;
	}
	if (Elapsed < 2.0f || Deaths >= 2)
	{
		return;
	}
	if (Deaths == 0)
	{
		Progress->AddExperience(160);
	}
	PreviousPawn = Pawn;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Killer = Player->GetWorld()->SpawnActor<AArenaMonster>(AArenaMonster::StaticClass(), Pawn->GetActorLocation() + FVector(100, 0, 0), FRotator::ZeroRotator, Params);
	if (Killer.IsValid())
	{
		Killer->MeleeDamage = 1000.0f;
		bWaitingForRespawn = true;
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaLifecycleTest: waiting for monster kill %d"), Deaths + 1);
	}
}
#endif
