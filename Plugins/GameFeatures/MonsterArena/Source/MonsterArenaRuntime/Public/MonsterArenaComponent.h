#pragma once

#include "Components/GameStateComponent.h"
#include "MonsterArenaComponent.generated.h"

class AArenaMonster;
class UMonsterArenaHUD;
class ULyraEquipmentInstance;
class ULyraInventoryItemInstance;
class ULyraInventoryManagerComponent;
class ULyraQuickBarComponent;

struct FArenaLoadout
{
	TWeakObjectPtr<ULyraQuickBarComponent> QuickBar;
	int32 SlotIndex = INDEX_NONE;
	bool bOwnsItem = false;
	TWeakObjectPtr<ULyraInventoryItemInstance> Item;
	TWeakObjectPtr<ULyraInventoryManagerComponent> Inventory;
};

/** Added by the experience, never by a global startup hook. */
UCLASS(meta = (BlueprintSpawnableComponent))
class MONSTERARENARUNTIME_API UMonsterArenaComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UMonsterArenaComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "1"))
	int32 MaxAliveMonsters = 6;
	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "0.1"))
	float SpawnInterval = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Arena")
	TSubclassOf<AArenaMonster> MonsterClass;
	UPROPERTY(EditAnywhere, Category = "Arena|UI")
	TSoftClassPtr<UMonsterArenaHUD> HUDClass = TSoftClassPtr<UMonsterArenaHUD>(FSoftObjectPath(TEXT("/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD_C")));

private:
	float SpawnCountdown = 2.0f;
	TArray<TWeakObjectPtr<AArenaMonster>> Monsters;
	TMap<TWeakObjectPtr<APawn>, FArenaLoadout> EquippedPawns;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMonsterArenaHUD>> HUDs;
};
