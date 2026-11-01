#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaRewardCardLibrary.generated.h"

class UBlueprint;

UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaRewardCardLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static UBlueprint* CreateStat();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static UBlueprint* CreateCard();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool ValidateStat(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool ValidateCard(UBlueprint* Blueprint);
};
