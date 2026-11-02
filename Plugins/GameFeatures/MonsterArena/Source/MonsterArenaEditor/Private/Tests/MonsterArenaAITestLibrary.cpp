#include "MonsterArenaAITestLibrary.h"

#include "ArenaMonster.h"
#include "Components/BoxComponent.h"
#include "Components/StateTreeComponent.h"
#include "Engine/World.h"

AArenaMonster* UMonsterArenaAITestLibrary::SpawnMonster(APawn* Player, FVector Offset)
{
	if (!IsValid(Player) || !Player->HasAuthority())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return Player->GetWorld()->SpawnActor<AArenaMonster>(AArenaMonster::StaticClass(), Player->GetActorLocation() + Offset, FRotator::ZeroRotator, Params);
}

FString UMonsterArenaAITestLibrary::GetAIState(AArenaMonster* Monster)
{
	if (!IsValid(Monster) || !Monster->StateTreeComponent->IsRunning())
	{
		return TEXT("Stopped");
	}
#if WITH_GAMEPLAY_DEBUGGER
	const TArray<FName> Names = Monster->StateTreeComponent->GetActiveStateNames();
	return Names.IsEmpty() ? TEXT("NoState") : Names.Last().ToString();
#else
	return TEXT("Running");
#endif
}

APawn* UMonsterArenaAITestLibrary::GetTarget(AArenaMonster* Monster)
{
	return IsValid(Monster) ? Monster->AITarget.Get() : nullptr;
}

AActor* UMonsterArenaAITestLibrary::SpawnMeleeBlocker(AArenaMonster* Monster, APawn* Player)
{
	if (!IsValid(Monster) || !IsValid(Player) || !Monster->HasAuthority())
	{
		return nullptr;
	}
	AActor* Blocker = Monster->GetWorld()->SpawnActor<AActor>();
	if (!Blocker)
	{
		return nullptr;
	}
	UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
	Blocker->SetRootComponent(Box);
	Blocker->AddInstanceComponent(Box);
	Box->SetBoxExtent(FVector(8.0f, 100.0f, 120.0f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Box->RegisterComponent();
	Blocker->SetActorLocation((Monster->GetActorLocation() + Player->GetActorLocation()) * 0.5f);
	return Blocker;
}
