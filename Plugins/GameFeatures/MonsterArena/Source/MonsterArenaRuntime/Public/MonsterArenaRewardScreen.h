#pragma once

#include "CommonActivatableWidget.h"
#include "InputAction.h"
#include "MonsterArenaRewardScreen.generated.h"

class UButton;
class UBorder;
class UTextBlock;
class UMonsterArenaHUD;
class URunProgressionComponent;

/** Native action with a stable mapping name, exposed in Lyra's keyboard settings. */
UCLASS()
class UMonsterArenaRewardInputAction : public UInputAction
{
	GENERATED_BODY()
public:
	UMonsterArenaRewardInputAction();
};

/** Per-local-player selection UI. Only the server can grant a card. */
UCLASS(Blueprintable)
class MONSTERARENARUNTIME_API UMonsterArenaRewardScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UMonsterArenaRewardScreen();
	void InitializeForProgression(URunProgressionComponent* InProgression, UMonsterArenaHUD* InHUD);
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void SelectCard(int32 Index);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void ConfirmSelection();
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void CloseSelection();
	bool IsAwaitingResponse() const { return bAwaitingResponse; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual bool NativeOnHandleBackAction() override;

private:
	UFUNCTION()
	void RefreshChoices();
	UFUNCTION()
	void HandleRequestFailed(FText Reason);
	UFUNCTION()
	void SelectFirst();
	UFUNCTION()
	void SelectSecond();
	UFUNCTION()
	void SelectThird();
	void ReleaseSession();
	void RefreshSelectionStyle();
	UPROPERTY(Transient)
	TObjectPtr<URunProgressionComponent> Progression;
	UPROPERTY(Transient)
	TObjectPtr<UMonsterArenaHUD> HUD;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> CardButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> CardBorders;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardTitles;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardDescriptions;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardPreviews;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardStacks;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PendingText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;
	int32 SelectedIndex = INDEX_NONE;
	int32 DisplayedRunId = INDEX_NONE;
	int32 DisplayedOfferId = INDEX_NONE;
	bool bAwaitingResponse = false;
	bool bSessionActive = false;
};
