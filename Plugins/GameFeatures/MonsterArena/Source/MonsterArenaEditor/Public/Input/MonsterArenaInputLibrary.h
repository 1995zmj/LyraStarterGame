#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaInputLibrary.generated.h"

class UInputMappingContext;

/** Editor-only creation and validation of the MonsterArena input assets. */
UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Creates missing actions and the mapping context; existing designer edits are preserved. */
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static UInputMappingContext* CreateInputAssets();

	/** Verifies asset classes, value types, player-mappable settings and default key mappings. */
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool ValidateInputAssets();
};
