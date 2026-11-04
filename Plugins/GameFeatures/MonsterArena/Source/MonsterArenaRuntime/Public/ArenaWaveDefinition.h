#pragma once

#include "Engine/DataAsset.h"
#include "ArenaMonsterConfig.h"
#include "ArenaWaveDefinition.generated.h"

/** One monster type selected by a wave; the type's values live in the shared monster catalog. */
USTRUCT(BlueprintType)
struct MONSTERARENARUNTIME_API FArenaWaveMonsterEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName MonsterId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1")) int32 Count = 1;
	/** Hunters prioritize players; other roles march on the defense core. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHunter = false;
	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct MONSTERARENARUNTIME_API FArenaWaveConfig
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FArenaWaveMonsterEntry> Monsters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AliveLimit = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BatchSize = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BatchInterval = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EntranceCount = 1;
	bool IsValid() const;
	int32 GetMonsterCount() const;
};

UCLASS(BlueprintType)
class MONSTERARENARUNTIME_API UArenaWaveDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UArenaWaveDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FArenaWaveConfig> Waves;
	/** Monster catalog referenced by MonsterId from each wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FArenaMonsterConfig> Monsters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WarningDuration = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WithinBatchInterval = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PlayerSafeDistance = 350.0f;
	/** Returns null when the catalog does not contain a valid entry for the id. */
	const FArenaMonsterConfig* FindMonster(FName MonsterId) const;
};
