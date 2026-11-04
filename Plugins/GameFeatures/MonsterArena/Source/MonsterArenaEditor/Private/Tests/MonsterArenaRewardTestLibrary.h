#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MonsterArenaRewardTestLibrary.generated.h"

class APlayerController;
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
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool DealAttributedDamage(APawn* Source, APawn* Target, float Amount);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool KillTestMonster(APawn* Source);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool UseAccumulationPool(URunProgressionComponent* Progression);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static bool SetTestTeam(APawn* Pawn, int32 Team);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Tests")
	static void RepeatElimination(APawn* Source, APawn* Target);
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static float GetBaseDamage(APawn* Pawn);

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

	/** True when the local player registered IMC_MonsterArena and U is mapped to open rewards. */
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool HasRewardInputMapping(APlayerController* Controller);

	/** True when the owning player's ASC was granted the input-driven reward ability. */
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool HasRewardAbility(APlayerController* Controller);

	/** True when the local player maps N to IA_MonsterArena_WaveAction. */
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool HasWaveInputMapping(APlayerController* Controller);

	/** True when the owning player's ASC was granted the input-driven wave ability. */
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool HasWaveAbility(APlayerController* Controller);

	/** True when the mapping name is present in the local player's mappable key profile. */
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool HasPlayerMappableSetting(APlayerController* Controller, FName MappingName);

	UFUNCTION(BlueprintPure, Category = "MonsterArena|Tests")
	static bool IsDamageImmune(APawn* Pawn);
};
