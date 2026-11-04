#pragma once

#include "Blueprint/UserWidget.h"
#include "MonsterArenaHUD.generated.h"

class UTextBlock;
class UProgressBar;
class URunProgressionComponent;
class ULyraAbilitySystemComponent;
class UMonsterArenaCoreHealthWidget;
class UMonsterArenaRewardScreen;
class UInputAction;
class UInputMappingContext;
class FMonsterArenaRewardInputTracker;

/** Blueprint owns the HUD layout; C++ updates progression and manages reward interaction. */
UCLASS(Abstract, Blueprintable)
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
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|HUD", meta = (BindWidget))
	TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|HUD", meta = (BindWidget))
	TObjectPtr<UTextBlock> Reward;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|HUD", meta = (BindWidget))
	TObjectPtr<UProgressBar> ExperienceBar;
	/** Designer-placed child inside CoreHealthSlot; no runtime creation. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UMonsterArenaCoreHealthWidget> CoreHealthDisplay;
	UPROPERTY(EditDefaultsOnly, Category = "MonsterArena|Rewards")
	TSoftClassPtr<UMonsterArenaRewardScreen> RewardScreenClass = TSoftClassPtr<UMonsterArenaRewardScreen>(FSoftObjectPath(TEXT("/MonsterArena/UI/W_MonsterArenaRewardScreen.W_MonsterArenaRewardScreen_C")));
	UPROPERTY(Transient)
	TObjectPtr<UMonsterArenaRewardScreen> RewardScreen;
	UPROPERTY(Transient)
	TObjectPtr<URunProgressionComponent> BoundProgression;
	/** Registered with the local player's user settings so both bindings appear in the settings screen. */
	UPROPERTY(EditDefaultsOnly, Category = "MonsterArena|Input")
	TSoftObjectPtr<UInputMappingContext> RewardMappingAsset = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/MonsterArena/Input/Mappings/IMC_MonsterArena.IMC_MonsterArena")));
	/** Reward action is used for the input hint and the reward screen's close-key query. */
	UPROPERTY(EditDefaultsOnly, Category = "MonsterArena|Input")
	TSoftObjectPtr<UInputAction> OpenRewardActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/MonsterArena/Input/Actions/IA_MonsterArena_OpenRewards.IA_MonsterArena_OpenRewards")));
	/** Wave action is only read to display the current shortcut; the ability handles the input. */
	UPROPERTY(EditDefaultsOnly, Category = "MonsterArena|Input")
	TSoftObjectPtr<UInputAction> WaveActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/MonsterArena/Input/Actions/IA_MonsterArena_WaveAction.IA_MonsterArena_WaveAction")));
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> OpenRewardAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> WaveAction;
	int32 LastLevel = 1;
	float NotificationRemaining = 0.0f;
private:
	UFUNCTION()
	void RefreshProgression();
	void RefreshCoreHealth();
	void TryOpenRewardMenu();
	void ReleaseRewardInputBlock();
	void HandleApplicationActivationChanged(bool bActive);
	TWeakObjectPtr<ULyraAbilitySystemComponent> BlockedASC;
	TWeakObjectPtr<APawn> InputPawn;
	TSharedPtr<FMonsterArenaRewardInputTracker> InputTracker;
	bool bWaitForInputRelease = false;
	bool bViewportActive = true;
	bool bRewardOpenRequested = false;
};
