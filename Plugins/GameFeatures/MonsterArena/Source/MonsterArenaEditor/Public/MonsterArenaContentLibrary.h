#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaContentLibrary.generated.h"

class ULyraExperienceDefinition;

/** Editor scripting bridge for authoring MonsterArena assets; never used by a running match. */
UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaContentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static bool BindProgression(ULyraExperienceDefinition* Experience, TSubclassOf<UActorComponent> ComponentClass);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static void ConfigureExperience(ULyraExperienceDefinition* Experience);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static void ConfigureWorld(UWorld* World);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Setup")
	static void ConfigureEditorAssetScope();
};
