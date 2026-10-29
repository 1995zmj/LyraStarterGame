#pragma once

#include "Components/GameStateComponent.h"
#include "Blueprint/UserWidget.h"
#include "MonsterArenaComponent.generated.h"

class AArenaMonster;
class UTextBlock;
class UProgressBar;
class URunProgressionComponent;
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

UCLASS()
class MONSTERARENARUNTIME_API UMonsterArenaHUD : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Reward;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ExperienceBar;
	int32 LastLevel = 1;
	float NotificationRemaining = 0.0f;
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

private:
	float SpawnCountdown = 2.0f;
	TArray<TWeakObjectPtr<AArenaMonster>> Monsters;
	TMap<TWeakObjectPtr<APawn>, FArenaLoadout> EquippedPawns;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMonsterArenaHUD>> HUDs;
};
