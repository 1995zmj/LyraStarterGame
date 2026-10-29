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
class ULyraAbilitySystemComponent;
class UMonsterArenaRewardScreen;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;
class FMonsterArenaRewardInputTracker;

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
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void OpenRewardMenu();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void CloseRewardMenu();
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Rewards")
	bool IsRewardMenuOpen() const;
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Rewards")
	UMonsterArenaRewardScreen* GetRewardScreen() const { return RewardScreen; }
	void BeginRewardInputBlock();
	void EndRewardInputBlock();
	bool IsRewardToggleKey(const FKey& Key) const;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Reward;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ExperienceBar;
	UPROPERTY(EditDefaultsOnly, Category = "MonsterArena|Rewards")
	TSubclassOf<UMonsterArenaRewardScreen> RewardScreenClass;
	UPROPERTY(Transient)
	TObjectPtr<UMonsterArenaRewardScreen> RewardScreen;
	UPROPERTY(Transient)
	TObjectPtr<URunProgressionComponent> BoundProgression;
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RewardMapping;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> OpenRewardAction;
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> RewardInput;
	int32 LastLevel = 1;
	float NotificationRemaining = 0.0f;
private:
	UFUNCTION()
	void RefreshProgression();
	void ToggleRewardMenu();
	void TryOpenRewardMenu();
	void EnsureInputMapping();
	void ReleaseRewardInputBlock();
	void HandleApplicationActivationChanged(bool bActive);
	TWeakObjectPtr<ULyraAbilitySystemComponent> BlockedASC;
	TWeakObjectPtr<APawn> InputPawn;
	TSharedPtr<FMonsterArenaRewardInputTracker> InputTracker;
	bool bWaitForInputRelease = false;
	bool bViewportActive = true;
	bool bRewardOpenRequested = false;
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
