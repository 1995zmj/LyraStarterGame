#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaAITestLibrary.generated.h"

class AArenaMonster;

UCLASS()
class UMonsterArenaAITestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static AArenaMonster* SpawnMonster(APawn* Player, FVector Offset);
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static FString GetAIState(AArenaMonster* Monster);
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static APawn* GetTarget(AArenaMonster* Monster);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static AActor* SpawnMeleeBlocker(AArenaMonster* Monster, APawn* Player);
};
