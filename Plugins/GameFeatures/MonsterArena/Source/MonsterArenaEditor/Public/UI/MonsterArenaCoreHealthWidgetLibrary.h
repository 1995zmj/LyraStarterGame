#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaCoreHealthWidgetLibrary.generated.h"

class UBlueprint;

/** Editor-only creation and validation of the separate core-health widget. */
UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaCoreHealthWidgetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static UBlueprint* CreateCoreHealthWidget();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool ValidateCoreHealthWidget(UBlueprint* Blueprint);
};
