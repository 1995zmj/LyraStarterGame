#pragma once

#include "CommonActivatableWidget.h"
#include "MonsterArenaRewardScreen.generated.h"

class UButton;
class UMonsterArenaRewardCard;
class UTextBlock;
class UMonsterArenaHUD;
class URunProgressionComponent;

/** Per-local-player selection UI. Only the server can grant a card. */
UCLASS(Abstract, Blueprintable)
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
	virtual void NativeConstruct() override;
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
	void HandleCardSelected(int32 Index);
	void ReleaseSession();
	void RefreshSelectionStyle();
	UPROPERTY(Transient)
	TObjectPtr<URunProgressionComponent> Progression;
	UPROPERTY(Transient)
	TObjectPtr<UMonsterArenaHUD> HUD;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|Rewards", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UMonsterArenaRewardCard>> Cards;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> PendingText;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> FeedbackText;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UMonsterArenaRewardCard> Card1;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UMonsterArenaRewardCard> Card2;
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UMonsterArenaRewardCard> Card3;
	int32 SelectedIndex = INDEX_NONE;
	int32 DisplayedRunId = INDEX_NONE;
	int32 DisplayedOfferId = INDEX_NONE;
	bool bAwaitingResponse = false;
	bool bSessionActive = false;
};
