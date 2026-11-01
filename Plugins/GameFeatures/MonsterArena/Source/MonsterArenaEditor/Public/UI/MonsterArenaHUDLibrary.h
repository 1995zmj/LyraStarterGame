#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaHUDLibrary.generated.h"

class UBlueprint;

/** Editor-only creation and validation of the designer-authored HUD. */
UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaHUDLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static UBlueprint* CreateHUD();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool ValidateHUD(UBlueprint* Blueprint);
};
