#include "ArenaWaveDirectorComponent.h"
#include "ArenaDefenseCore.h"
#include "ArenaMonster.h"
#include "MonsterArenaComponent.h"
#include "RunProgressionComponent.h"
#include "Character/LyraHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Net/UnrealNetwork.h"

UArenaWaveDirectorComponent::UArenaWaveDirectorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	MonsterClass = AArenaMonster::StaticClass();
}

void UArenaWaveDirectorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()->HasAuthority()) { return; }
	if (!Definition) { Definition = NewObject<UArenaWaveDefinition>(this); }
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MonsterSpawn"))) { Entrances.Add(*It); }
	}
	Entrances.Sort([](const TWeakObjectPtr<AActor>& A, const TWeakObjectPtr<AActor>& B) { return A->GetName() < B->GetName(); });
	for (TActorIterator<AArenaDefenseCore> It(GetWorld()); It; ++It) { Core = *It; break; }
	State.WaveCount = Definition->Waves.Num();
	Publish();
}

void UArenaWaveDirectorComponent::Publish()
{
	State.AliveCount = Living.Num();
	State.PendingCount = Queue.Num();
	GetOwner()->ForceNetUpdate();
}

bool UArenaWaveDirectorComponent::RequestNextWave(APawn* Requester, int32 ExpectedRunId, int32 ExpectedWaveIndex)
{
	const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Requester);
	if (!GetOwner()->HasAuthority() || !Requester || Requester->GetWorld() != GetWorld() || !Requester->IsPlayerControlled()
		|| !Health || Health->IsDeadOrDying() || Health->GetHealth() <= 0 || !Core || Core->Health <= 0
		|| FVector::DistSquared(Requester->GetActorLocation(), Core->GetActorLocation()) > FMath::Square(450.0f)
		|| ExpectedRunId != State.RunId || ExpectedWaveIndex != State.WaveIndex
		|| (State.Phase != EArenaWavePhase::Preparation && State.Phase != EArenaWavePhase::Intermission)) { return false; }
	if (!Definition || !Definition->Waves.IsValidIndex(State.WaveIndex)) { return false; }
	CurrentWave = Definition->Waves[State.WaveIndex];
	if (!CurrentWave.IsValid() || !FMath::IsFinite(Definition->WarningDuration) || Definition->WarningDuration < 0
		|| !FMath::IsFinite(Definition->WithinBatchInterval) || Definition->WithinBatchInterval < 0.1f
		|| !FMath::IsFinite(Definition->PlayerSafeDistance) || Definition->PlayerSafeDistance < 0) { return false; }
	Entrances.RemoveAll([](auto Entrance) { return !Entrance.IsValid(); });
	if (Entrances.Num() < CurrentWave.EntranceCount)
	{
		State.bSpawnBlocked = true; Publish();
		UE_LOG(LogTemp, Warning, TEXT("MonsterArena: wave requires %d entrances, found %d"), CurrentWave.EntranceCount, Entrances.Num());
		return false;
	}
	WaveEntrances.Reset();
	State.EntranceNames.Reset();
	for (int32 Index = 0; Index < CurrentWave.EntranceCount; ++Index)
	{
		AActor* Entrance = Entrances[(State.WaveIndex + Index) % Entrances.Num()].Get();
		WaveEntrances.Add(Entrance);
		const FVector Offset = Entrance->GetActorLocation() - Core->GetActorLocation();
		const TCHAR* Direction = FMath::Abs(Offset.X) > FMath::Abs(Offset.Y) ? (Offset.X > 0 ? TEXT("+X") : TEXT("-X")) : (Offset.Y > 0 ? TEXT("+Y") : TEXT("-Y"));
		if (!State.EntranceNames.IsEmpty()) { State.EntranceNames += TEXT(" / "); }
		State.EntranceNames += Direction;
	}
	// Bresenham-style distribution keeps hunters interleaved, including short final batches.
	Queue.Reset();
	const int32 Total = CurrentWave.AssaultCount + CurrentWave.HunterCount;
	for (int32 Index = Total - 1; Index >= 0; --Index)
	{
		Queue.Add(((Index + 1) * CurrentWave.HunterCount / Total) > (Index * CurrentWave.HunterCount / Total));
	}
	++State.WaveIndex;
	State.Phase = EArenaWavePhase::Warning;
	State.bSpawnBlocked = false;
	FailedSince = -1;
	EntranceCursor = 0;
	BatchRemaining = 0;
	NextSpawnTime = GetWorld()->GetTimeSeconds() + Definition->WarningDuration;
	Publish();
	return true;
}

bool UArenaWaveDirectorComponent::TrySpawn(bool bHunter)
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav || !Core || !MonsterClass || WaveEntrances.IsEmpty()) { return false; }
	const AArenaMonster* Defaults = MonsterClass->GetDefaultObject<AArenaMonster>();
	const float HalfHeight = Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Radius = Defaults->GetCapsuleComponent()->GetScaledCapsuleRadius();
	for (int32 Attempt = 0; Attempt < WaveEntrances.Num() * 4; ++Attempt)
	{
		const int32 EntranceIndex = (EntranceCursor + Attempt / 4) % WaveEntrances.Num();
		AActor* Entrance = WaveEntrances[EntranceIndex].Get();
		if (!Entrance) { continue; }
		FNavLocation Location;
		const FVector Offset(Attempt % 4 == 0 ? 0 : FMath::FRandRange(-200.0f, 200.0f), Attempt % 4 == 0 ? 0 : FMath::FRandRange(-200.0f, 200.0f), 0);
		if (!Nav->ProjectPointToNavigation(Entrance->GetActorLocation() + Offset, Location, FVector(100, 100, 250))) { continue; }
		const FVector SpawnLocation = Location.Location + FVector(0, 0, HalfHeight + 3);
		bool bUnsafe = false;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APawn* Player = It->Get() ? It->Get()->GetPawn() : nullptr;
			if (Player && FVector::DistSquared(Player->GetActorLocation(), SpawnLocation) < FMath::Square(Definition->PlayerSafeDistance)) { bUnsafe = true; break; }
		}
		if (bUnsafe || GetWorld()->OverlapBlockingTestByChannel(SpawnLocation, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight))) { continue; }
		UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Location.Location, Core->GetActorLocation());
		if (!Path || !Path->IsValid() || Path->IsPartial()) { continue; }
		FTransform Transform(Entrance->GetActorRotation(), SpawnLocation);
		AArenaMonster* Monster = GetWorld()->SpawnActorDeferred<AArenaMonster>(MonsterClass, Transform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
		if (!Monster) { continue; }
		Monster->DefenseTarget = Core;
		Monster->bHunter = bHunter;
		Monster->WaveDirector = this;
		Monster->FinishSpawning(Transform);
		if (!IsValid(Monster)) { continue; }
		Living.Add(Monster, bHunter);
		Monster->OnDestroyed.AddDynamic(this, &ThisClass::OnMonsterDestroyed);
		EntranceCursor = (EntranceIndex + 1) % WaveEntrances.Num();
		return true;
	}
	return false;
}

void UArenaWaveDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	if (!GetOwner()->HasAuthority()) { return; }
	if (State.Phase == EArenaWavePhase::Victory || State.Phase == EArenaWavePhase::Defeat) { return; }
	if (Core && Core->Health <= 0) { FailRun(); return; }
	// Administrative test pause remains compatible with the existing test harness.
	if (auto* Arena = GetOwner()->FindComponentByClass<UMonsterArenaComponent>()) { if (Arena->MaxAliveMonsters <= 0) { return; } }
	if (State.Phase == EArenaWavePhase::Preparation || State.Phase == EArenaWavePhase::Intermission) { return; }
	if (!Core) { FailRun(); return; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (State.Phase == EArenaWavePhase::Warning && Now >= NextSpawnTime) { State.Phase = EArenaWavePhase::Spawning; Publish(); }
	if (State.Phase == EArenaWavePhase::Spawning && Now >= NextSpawnTime && Living.Num() < CurrentWave.AliveLimit && !Queue.IsEmpty())
	{
		if (BatchRemaining == 0) { BatchRemaining = FMath::Min(CurrentWave.BatchSize, Queue.Num()); }
		if (TrySpawn(Queue.Last()))
		{
			Queue.Pop();
			--BatchRemaining;
			NextSpawnTime = Now + (BatchRemaining > 0 ? Definition->WithinBatchInterval : CurrentWave.BatchInterval);
			FailedSince = -1; State.bSpawnBlocked = false;
			if (Queue.IsEmpty()) { State.Phase = EArenaWavePhase::Clearing; }
			Publish();
		}
		else
		{
			NextSpawnTime = Now + 1.0;
			if (FailedSince < 0) { FailedSince = Now; }
			if (!State.bSpawnBlocked && Now - FailedSince >= 10)
			{
				State.bSpawnBlocked = true; Publish();
				UE_LOG(LogTemp, Warning, TEXT("MonsterArena: spawn blocked; pending quota retained, check navigation/entrances/player distance"));
			}
		}
	}
	if (State.Phase == EArenaWavePhase::Clearing && Living.IsEmpty() && Queue.IsEmpty())
	{
		State.Phase = State.WaveIndex == State.WaveCount ? EArenaWavePhase::Victory : EArenaWavePhase::Intermission;
		Publish();
	}
}

void UArenaWaveDirectorComponent::NotifyMonsterDeath(AArenaMonster* Monster)
{
	if (!GetOwner()->HasAuthority() || bCleaning) { return; }
	if (Living.Remove(Monster) > 0) { Publish(); }
}

void UArenaWaveDirectorComponent::OnMonsterDestroyed(AActor* Actor)
{
	AArenaMonster* Monster = Cast<AArenaMonster>(Actor);
	if (!GetOwner()->HasAuthority() || bCleaning) { return; }
	if (const bool* Hunter = Living.Find(Monster))
	{
		Queue.Add(*Hunter); Living.Remove(Monster);
		if (State.Phase == EArenaWavePhase::Clearing) { State.Phase = EArenaWavePhase::Spawning; }
		Publish();
	}
}

void UArenaWaveDirectorComponent::CleanupMonsters()
{
	bCleaning = true;
	TArray<TWeakObjectPtr<AArenaMonster>> Monsters;
	Living.GetKeys(Monsters);
	Living.Reset(); Queue.Reset();
	for (auto Monster : Monsters)
	{
		if (Monster.IsValid())
		{
			Monster->OnDestroyed.RemoveDynamic(this, &ThisClass::OnMonsterDestroyed);
			Monster->WaveDirector.Reset();
			Monster->Destroy();
		}
	}
	bCleaning = false;
}

void UArenaWaveDirectorComponent::FailRun()
{
	if (!GetOwner()->HasAuthority() || State.Phase == EArenaWavePhase::Defeat || State.Phase == EArenaWavePhase::Victory) { return; }
	State.Phase = EArenaWavePhase::Defeat;
	CleanupMonsters(); Publish();
}

void UArenaWaveDirectorComponent::ResetRun()
{
	if (!GetOwner()->HasAuthority()) { return; }
	CleanupMonsters();
	const int32 NewRunId = State.RunId + 1;
	State = FArenaWaveState(); State.RunId = NewRunId;
	State.WaveCount = Definition ? Definition->Waves.Num() : 0;
	BatchRemaining = 0; NextSpawnTime = 0; FailedSince = -1;
	if (Core) { Core->ResetCore(); }
	for (APlayerState* PS : GetWorld()->GetGameState()->PlayerArray)
	{
		if (auto* Progress = PS->FindComponentByClass<URunProgressionComponent>()) { Progress->ResetRun(); }
	}
	Publish();
}

void UArenaWaveDirectorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetOwner()->HasAuthority()) { CleanupMonsters(); }
	Super::EndPlay(Reason);
}

void UArenaWaveDirectorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UArenaWaveDirectorComponent, State);
	DOREPLIFETIME(UArenaWaveDirectorComponent, Core);
}
