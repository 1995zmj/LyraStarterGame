#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaWaveTestLibrary.generated.h"
class URunProgressionComponent;
class UArenaWaveDirectorComponent;

UCLASS()
class UMonsterArenaWaveTestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable) static void RequestWave(URunProgressionComponent* Player, int32 RunId, int32 WaveIndex);
	UFUNCTION(BlueprintCallable) static void DamageCore(UArenaWaveDirectorComponent* Director, float Amount);
	UFUNCTION(BlueprintCallable) static void UseShortWaves(UArenaWaveDirectorComponent* Director);
};
