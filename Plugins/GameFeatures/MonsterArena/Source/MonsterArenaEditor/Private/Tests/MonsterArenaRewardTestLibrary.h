#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaRewardTestLibrary.generated.h"

class UMonsterArenaHUD;
class UMonsterArenaRewardScreen;
class URunProgressionComponent;

/** Editor-only entry points for the asynchronous multiplayer reward regression script. */
UCLASS()
class UMonsterArenaRewardTestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool StartSession(int32 PlayerCount = 2);

	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static TArray<UWorld*> GetTestWorlds();

	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static void StopSession();

	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool DealDamage(APawn* Pawn, float Amount);

	/** Dispatches normal gameplay entry points without Python's editor-only RPC callspace override. */
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool OpenRewardMenu(UMonsterArenaHUD* HUD);

	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool CloseRewardMenu(UMonsterArenaHUD* HUD);

	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool SelectAndConfirmReward(UMonsterArenaRewardScreen* Screen, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool RequestCard(URunProgressionComponent* Progression, int32 RunId, int32 OfferId, FName CardId);

	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static int32 GetInputBlockCount(APawn* Pawn);

	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool IsDamageImmune(APawn* Pawn);
};
