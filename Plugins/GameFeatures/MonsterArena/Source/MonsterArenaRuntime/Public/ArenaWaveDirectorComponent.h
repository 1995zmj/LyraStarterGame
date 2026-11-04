#pragma once

#include "Components/GameStateComponent.h"
#include "ArenaWaveDefinition.h"
#include "ArenaWaveDirectorComponent.generated.h"

class AArenaMonster;
class AArenaDefenseCore;

UENUM(BlueprintType)
enum class EArenaWavePhase : uint8 { Preparation, Warning, Spawning, Clearing, Intermission, Victory, Defeat };

/** Server-only spawn ticket; requeued monsters keep the type and role they were scheduled with. */
struct FArenaSpawnRequest
{
	FName MonsterId;
	bool bHunter = false;
};

USTRUCT(BlueprintType)
struct FArenaWaveState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 RunId = 1;
	UPROPERTY(BlueprintReadOnly) int32 WaveIndex = 0;
	UPROPERTY(BlueprintReadOnly) int32 WaveCount = 5;
	UPROPERTY(BlueprintReadOnly) int32 PendingCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 AliveCount = 0;
	UPROPERTY(BlueprintReadOnly) EArenaWavePhase Phase = EArenaWavePhase::Preparation;
	UPROPERTY(BlueprintReadOnly) bool bSpawnBlocked = false;
	UPROPERTY(BlueprintReadOnly) FString EntranceNames;
};

/** Authority owns the queue; clients only receive the public wave snapshot. */
UCLASS(meta = (BlueprintSpawnableComponent))
class MONSTERARENARUNTIME_API UArenaWaveDirectorComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UArenaWaveDirectorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UArenaWaveDefinition> Definition;
	UPROPERTY(Replicated, BlueprintReadOnly) FArenaWaveState State;
	UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<AArenaDefenseCore> Core;
	UPROPERTY(EditAnywhere) TSubclassOf<AArenaMonster> MonsterClass;
	bool RequestNextWave(APawn* Requester, int32 ExpectedRunId, int32 ExpectedWaveIndex);
	void FailRun();
	void NotifyMonsterDeath(AArenaMonster* Monster);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly) void ResetRun();
private:
	TArray<TWeakObjectPtr<AActor>> Entrances;
	TArray<TWeakObjectPtr<AActor>> WaveEntrances;
	TMap<TWeakObjectPtr<AArenaMonster>, FArenaSpawnRequest> Living;
	TArray<FArenaSpawnRequest> Queue;
	FArenaWaveConfig CurrentWave;
	int32 EntranceCursor = 0;
	int32 BatchRemaining = 0;
	double NextSpawnTime = 0;
	double FailedSince = -1;
	bool bCleaning = false;
	void Publish();
	void CleanupMonsters();
	bool TrySpawn(const FArenaSpawnRequest& Request);
	UFUNCTION() void OnMonsterDestroyed(AActor* Actor);
};
