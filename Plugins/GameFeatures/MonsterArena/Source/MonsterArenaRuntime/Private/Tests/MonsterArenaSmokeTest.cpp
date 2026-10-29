#include "ArenaMonster.h"
#include "RunProgressionComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "Character/LyraHealthComponent.h"
#include "Player/LyraPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

#if WITH_DEV_AUTOMATION_TESTS
// Runs only in a separately launched test game with -MonsterArenaSmokeTest.
void RunMonsterArenaSmokeTest(UWorld* World, ALyraPlayerState* Player)
{
	bool bPassed = true;
	auto Verify = [&bPassed](bool bCondition, const TCHAR* Description) {
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaTest: %s: %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), Description);
		bPassed &= bCondition;
	};
	URunProgressionComponent* Progress = Player->FindComponentByClass<URunProgressionComponent>();
	ULyraAbilitySystemComponent* ASC = Player->GetLyraAbilitySystemComponent();
	Verify(Progress && ASC && Player->GetPawn(), TEXT("Experience injected progression and initialized player GAS"));
	Verify(Player->GetPawn() && Player->GetPawn()->GetClass()->GetPathName().StartsWith(TEXT("/MonsterArena/")), TEXT("Experience uses MonsterArena-owned hero"));
	UClass* SharedHeroClass = LoadClass<APawn>(nullptr, TEXT("/ShooterCore/Game/B_Hero_ShooterMannequin.B_Hero_ShooterMannequin_C"));
	Verify(Player->GetPawn() && SharedHeroClass && Player->GetPawn()->IsA(SharedHeroClass), TEXT("Local hero preserves shared weapon pawn-type contract"));
	UClass* LocalRelayClass = LoadClass<UActorComponent>(nullptr, TEXT("/MonsterArena/UI/Notifications/B_MonsterArenaEliminationFeedRelay.B_MonsterArenaEliminationFeedRelay_C"));
	UClass* SharedRelayClass = LoadClass<UActorComponent>(nullptr, TEXT("/ShooterCore/Accolades/B_EliminationFeedRelay.B_EliminationFeedRelay_C"));
	TArray<UActorComponent*> LocalRelays;
	TArray<UActorComponent*> SharedRelays;
	if (World->GetGameState() && LocalRelayClass && SharedRelayClass)
	{
		World->GetGameState()->GetComponents(LocalRelayClass, LocalRelays);
		World->GetGameState()->GetComponents(SharedRelayClass, SharedRelays);
	}
	Verify(LocalRelays.Num() == 1 && SharedRelays.IsEmpty(), TEXT("Only the MonsterArena elimination relay is injected"));
	UClass* RifleEffect = LoadClass<UGameplayEffect>(nullptr, TEXT("/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto.GE_Damage_RifleAuto_C"));
	Verify(RifleEffect != nullptr, TEXT("ShooterCore rifle damage effect available"));
	if (Progress && ASC && Player->GetPawn() && RifleEffect)
	{
		Progress->ResetRun();
		const float BaseMaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
		const float BaseDamage = ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute());
		for (int32 Kill = 0; Kill < 8; ++Kill)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AArenaMonster* Monster = World->SpawnActor<AArenaMonster>(AArenaMonster::StaticClass(), Player->GetPawn()->GetActorLocation() + FVector(500, 0, 0), FRotator::ZeroRotator, Params);
			Verify(Monster && Monster->HealthComponent->GetHealth() == 60.0f, TEXT("Monster starts with configured GAS health"));
			if (!Monster)
			{
				break;
			}
			if (Kill == 0)
			{
				FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(MonsterArenaTest), false, Player->GetPawn());
				FHitResult Hit;
				World->LineTraceSingleByChannel(Hit, Player->GetPawn()->GetActorLocation(), Monster->GetActorLocation(), ECC_GameTraceChannel2, TraceParams);
				Verify(Hit.GetActor() == Monster, TEXT("Rifle trace channel hits monster collision"));
			}
			for (int32 Shot = 0; Shot < 6; ++Shot)
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				Context.AddOrigin(Player->GetPawn()->GetActorLocation());
				FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(RifleEffect, 1.0f, Context);
				ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, Monster->GetAbilitySystemComponent());
				if (Kill == 0 && Shot == 0)
				{
					Verify(FMath::IsNearlyEqual(Monster->HealthComponent->GetHealth(), 48.0f), TEXT("Real rifle execution deals 12 damage to enemy team"));
				}
			}
			Verify(Progress->State.Kills == Kill + 1, TEXT("Death reward settles once despite repeated lethal hits"));
			Monster->Destroy();
		}
		Verify(Progress->State.Level == 3 && Progress->State.Experience == 0, TEXT("8 kills award 160 XP and reach level 3"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), BaseMaxHealth + 20.0f), TEXT("Level 2 grants 20 maximum health"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), BaseDamage + 3.0f), TEXT("Level 3 grants 3 rifle base damage"));
		FActorSpawnParameters TargetParams;
		TargetParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AArenaMonster* UpgradedTarget = World->SpawnActor<AArenaMonster>(AArenaMonster::StaticClass(), Player->GetPawn()->GetActorLocation() + FVector(500, 0, 0), FRotator::ZeroRotator, TargetParams);
		if (UpgradedTarget)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddOrigin(Player->GetPawn()->GetActorLocation());
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(RifleEffect, 1.0f, Context);
			ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, UpgradedTarget->GetAbilitySystemComponent());
			Verify(FMath::IsNearlyEqual(UpgradedTarget->HealthComponent->GetHealth(), 45.0f), TEXT("Upgraded rifle execution deals 15 damage"));
			UpgradedTarget->Destroy();
		}
		Progress->ResetRun();
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()), BaseMaxHealth), TEXT("Reset removes health reward"));
		Verify(FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute()), BaseDamage), TEXT("Reset removes damage reward"));
		Progress->AddExperience(-20);
		Verify(Progress->State.Experience == 0, TEXT("Negative XP ignored"));
		Progress->AddExperience(325);
		Verify(Progress->State.Level == 4 && Progress->State.Experience == 25, TEXT("Bulk XP crosses multiple levels and preserves overflow"));
		Progress->AddExperience(MAX_int32);
		Verify(Progress->State.Level == 10 && Progress->GetExperienceRequired() == 0 && Progress->State.Experience == 0, TEXT("Maximum level and large XP handled safely"));
		Progress->ResetRun();
	}
	UE_LOG(LogTemp, Display, TEXT("MONSTER_ARENA_SMOKE_%s"), bPassed ? TEXT("SUCCEEDED") : TEXT("FAILED"));
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
#endif
