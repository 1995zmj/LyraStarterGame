#pragma once

#include "Components/GameStateComponent.h"
#include "MonsterArenaComponent.generated.h"

class AArenaMonster;
class UArenaWaveDirectorComponent;
class UArenaWaveDefinition;
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
	virtual void BeginPlay() override;
	/** Legacy administrative spawn gate: <= 0 pauses spawning; positive values do not override wave caps. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	int32 MaxAliveMonsters = 6;
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use WaveDefinition batch intervals"))
	float SpawnInterval = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Arena")
	TSubclassOf<AArenaMonster> MonsterClass;
	UPROPERTY(EditAnywhere, Category = "Arena|Waves")
	TSoftObjectPtr<UArenaWaveDefinition> WaveDefinition = TSoftObjectPtr<UArenaWaveDefinition>(FSoftObjectPath(TEXT("/MonsterArena/Data/DA_ArenaWaves.DA_ArenaWaves")));
	UPROPERTY(EditAnywhere, Category = "Arena|UI")
	TSoftClassPtr<UMonsterArenaHUD> HUDClass = TSoftClassPtr<UMonsterArenaHUD>(FSoftObjectPath(TEXT("/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD_C")));

private:
	UPROPERTY(Transient) TObjectPtr<UArenaWaveDirectorComponent> WaveDirector;
	TMap<TWeakObjectPtr<APawn>, FArenaLoadout> EquippedPawns;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMonsterArenaHUD>> HUDs;
};
