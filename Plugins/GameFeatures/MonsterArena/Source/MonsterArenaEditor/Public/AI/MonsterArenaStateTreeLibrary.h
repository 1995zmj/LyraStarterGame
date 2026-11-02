#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaStateTreeLibrary.generated.h"

class UStateTree;

UCLASS()
class MONSTERARENAEDITOR_API UMonsterArenaStateTreeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Editor")
	static UStateTree* CreateMonsterStateTree();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Editor")
	static bool ValidateMonsterStateTree(UStateTree* Tree);
};
