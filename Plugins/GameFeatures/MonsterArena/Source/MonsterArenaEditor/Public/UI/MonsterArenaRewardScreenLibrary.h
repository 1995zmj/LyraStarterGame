#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaRewardScreenLibrary.generated.h"

class UBlueprint;

/** Editor-only creation and validation of the card-selection designer layout. */
UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaRewardScreenLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static UBlueprint* CreateRewardScreen();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool ValidateRewardScreen(UBlueprint* Blueprint);
};
