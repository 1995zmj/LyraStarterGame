#pragma once

#include "Blueprint/UserWidget.h"
#include "RunRewardCardDefinition.h"
#include "MonsterArenaRewardCard.generated.h"

class UButton;
class UBorder;
class UTextBlock;
class UVerticalBox;
class URunProgressionComponent;
class UMonsterArenaRewardStat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMonsterArenaCardSelected, int32, ChoiceIndex);

/** Reusable card view. It displays snapshots and emits a selection, never grants rewards. */
UCLASS(Abstract, Blueprintable)
class MONSTERARENARUNTIME_API UMonsterArenaRewardCard : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void Configure(const FRunRewardOption& Card, int32 InChoiceIndex, int32 StackCount, URunProgressionComponent* Progression);
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|Rewards")
	void SetSelected(bool bSelected);
	UFUNCTION(BlueprintPure, Category = "MonsterArena|Rewards")
	UButton* GetSelectionButton() const { return SelectButton; }
	UPROPERTY(BlueprintAssignable, Category = "MonsterArena|Rewards")
	FMonsterArenaCardSelected OnSelected;
protected:
	virtual void NativeOnInitialized() override;
	UPROPERTY(EditDefaultsOnly, Category = "MonsterArena|Rewards")
	TSoftClassPtr<UMonsterArenaRewardStat> EffectRowClass = TSoftClassPtr<UMonsterArenaRewardStat>(FSoftObjectPath(TEXT("/MonsterArena/UI/W_MonsterArenaRewardStat.W_MonsterArenaRewardStat_C")));
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterArena|Style")
	FLinearColor NormalColor = FLinearColor(0.035f, 0.055f, 0.09f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterArena|Style")
	FLinearColor SelectedColor = FLinearColor(0.045f, 0.25f, 0.23f);
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|Rewards")
	FName DisplayedCardId;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UBorder> CardBorder;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceNumber;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> CardTitle;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> CardDescription;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UTextBlock> CardStack;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|View", meta = (BindWidget))
	TObjectPtr<UVerticalBox> EffectsBox;
private:
	UFUNCTION()
	void HandleSelected();
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|Rewards", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UMonsterArenaRewardStat>> EffectRows;
	int32 ChoiceIndex = INDEX_NONE;
};
