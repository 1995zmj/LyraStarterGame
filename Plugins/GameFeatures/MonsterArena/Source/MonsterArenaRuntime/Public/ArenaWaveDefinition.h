#pragma once

#include "Engine/DataAsset.h"
#include "ArenaWaveDefinition.generated.h"

USTRUCT(BlueprintType)
struct MONSTERARENARUNTIME_API FArenaWaveConfig
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AssaultCount = 12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HunterCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AliveLimit = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BatchSize = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BatchInterval = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EntranceCount = 1;
	bool IsValid() const;
};

UCLASS(BlueprintType)
class MONSTERARENARUNTIME_API UArenaWaveDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UArenaWaveDefinition();
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FArenaWaveConfig> Waves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WarningDuration = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WithinBatchInterval = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PlayerSafeDistance = 350.0f;
};
