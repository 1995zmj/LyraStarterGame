#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaWaveContentLibrary.generated.h"

UCLASS()
class MONSTERARENAEDITOR_API UArenaWaveContentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable) static bool SetupDefenseMap(UWorld* World);
};
