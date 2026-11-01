#include "UI/MonsterArenaRewardScreen.h"

#include "UI/MonsterArenaHUD.h"
#include "UI/MonsterArenaRewardCard.h"
#include "RunProgressionComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewards"

UMonsterArenaRewardScreen::UMonsterArenaRewardScreen()
{
	bIsBackHandler = true;
	bIsModal = true;
	SetIsFocusable(true);
}

void UMonsterArenaRewardScreen::InitializeForProgression(URunProgressionComponent* InProgression, UMonsterArenaHUD* InHUD)
{
	Progression = InProgression;
	HUD = InHUD;
	DisplayedRunId = INDEX_NONE;
	DisplayedOfferId = INDEX_NONE;
	SelectedIndex = INDEX_NONE;
}

TOptional<FUIInputConfig> UMonsterArenaRewardScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UMonsterArenaRewardScreen::NativeConstruct()
{
	Super::NativeConstruct();
	Cards = {Card1, Card2, Card3};
	for (UMonsterArenaRewardCard* Card : Cards)
	{
		Card->OnSelected.AddUniqueDynamic(this, &ThisClass::HandleCardSelected);
	}
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ConfirmSelection);
	CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseSelection);
	RefreshChoices();
}

void UMonsterArenaRewardScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (!Progression || !HUD || !Progression->CanOpenRewardChoices())
	{
		DeactivateWidget();
		return;
	}
	bSessionActive = true;
	bAwaitingResponse = true;
	Progression->OnProgressionChanged.AddUniqueDynamic(this, &ThisClass::RefreshChoices);
	Progression->OnRewardRequestFailed.AddUniqueDynamic(this, &ThisClass::HandleRequestFailed);
	HUD->BeginRewardInputBlock();
	Progression->RequestOpenRewardChoices();
	RefreshChoices();
}

void UMonsterArenaRewardScreen::NativeOnDeactivated()
{
	ReleaseSession();
	Super::NativeOnDeactivated();
}

void UMonsterArenaRewardScreen::NativeDestruct()
{
	ReleaseSession();
	Super::NativeDestruct();
}

void UMonsterArenaRewardScreen::ReleaseSession()
{
	if (Progression)
	{
		Progression->OnProgressionChanged.RemoveDynamic(this, &ThisClass::RefreshChoices);
		Progression->OnRewardRequestFailed.RemoveDynamic(this, &ThisClass::HandleRequestFailed);
	}
	if (bSessionActive)
	{
		bSessionActive = false;
		if (Progression)
		{
			Progression->RequestCloseRewardChoices();
		}
		if (HUD)
		{
			HUD->EndRewardInputBlock();
		}
	}
	// CommonUI pools screens; do not retain a previous match's HUD or PlayerState.
	Progression = nullptr;
	HUD = nullptr;
}

void UMonsterArenaRewardScreen::RefreshChoices()
{
	if (!PendingText || !Progression)
	{
		return;
	}
	const FRunProgressionState& State = Progression->State;
	if (bSessionActive && (State.PendingChoices <= 0 || !Progression->CanOpenRewardChoices()))
	{
		CloseSelection();
		return;
	}
	PendingText->SetText(FText::Format(LOCTEXT("Pending", "等级 {0}  ·  可选强化 ×{1}"), FText::AsNumber(State.Level), FText::AsNumber(State.PendingChoices)));
	const bool bNewOffer = DisplayedRunId != State.RunId || DisplayedOfferId != State.OfferId;
	if (bNewOffer)
	{
		DisplayedRunId = State.RunId;
		DisplayedOfferId = State.OfferId;
		SelectedIndex = INDEX_NONE;
		bAwaitingResponse = false;
		FeedbackText->SetText(LOCTEXT("ChooseHint", "选择一张卡牌，然后确认。"));
	}
	if (State.bRewardMenuOpen && !State.Options.IsEmpty() && SelectedIndex == INDEX_NONE)
	{
		bAwaitingResponse = false;
	}
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const bool bHasCard = State.Options.IsValidIndex(Index);
		Cards[Index]->SetVisibility(bHasCard ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		Cards[Index]->SetIsEnabled(bHasCard && State.bRewardMenuOpen && !bAwaitingResponse);
		if (!bHasCard) { continue; }
		const FRunRewardOption& Option = State.Options[Index];
		int32 StackCount = 0;
		for (const FRunRewardGrant& Grant : State.ChosenRewards)
		{
			StackCount += Grant.Card.CardId == Option.CardId ? 1 : 0;
		}
		Cards[Index]->Configure(Option, Index, StackCount, Progression);
	}
	RefreshSelectionStyle();
	if (bNewOffer && State.bRewardMenuOpen && !State.Options.IsEmpty())
	{
		ClearFocusRestorationTarget();
		RequestRefreshFocus();
	}
}

void UMonsterArenaRewardScreen::HandleRequestFailed(FText Reason)
{
	bAwaitingResponse = false;
	RefreshChoices();
	if (FeedbackText) { FeedbackText->SetText(Reason); }
}

void UMonsterArenaRewardScreen::SelectCard(int32 Index)
{
	if (Progression && !bAwaitingResponse && Progression->State.bRewardMenuOpen && Progression->State.Options.IsValidIndex(Index))
	{
		SelectedIndex = Index;
		RefreshSelectionStyle();
	}
}

void UMonsterArenaRewardScreen::HandleCardSelected(int32 Index) { SelectCard(Index); }

void UMonsterArenaRewardScreen::RefreshSelectionStyle()
{
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		Cards[Index]->SetSelected(Index == SelectedIndex);
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetIsEnabled(!bAwaitingResponse && Progression && Progression->State.bRewardMenuOpen && Progression->State.Options.IsValidIndex(SelectedIndex));
	}
}

void UMonsterArenaRewardScreen::ConfirmSelection()
{
	if (!Progression || bAwaitingResponse || !Progression->State.Options.IsValidIndex(SelectedIndex)) { return; }
	const FName CardId = Progression->State.Options[SelectedIndex].CardId;
	bAwaitingResponse = true;
	RefreshSelectionStyle();
	FeedbackText->SetText(LOCTEXT("Confirming", "正在确认强化…"));
	for (UMonsterArenaRewardCard* Card : Cards) { Card->SetIsEnabled(false); }
	Progression->RequestSelectReward(DisplayedRunId, DisplayedOfferId, CardId);
}

void UMonsterArenaRewardScreen::CloseSelection() { DeactivateWidget(); }
bool UMonsterArenaRewardScreen::NativeOnHandleBackAction() { CloseSelection(); return true; }

UWidget* UMonsterArenaRewardScreen::NativeGetDesiredFocusTarget() const
{
	for (UMonsterArenaRewardCard* Card : Cards)
	{
		if (Card && Card->GetVisibility() == ESlateVisibility::Visible && Card->GetIsEnabled()) { return Card->GetSelectionButton(); }
	}
	return CloseButton;
}

FReply UMonsterArenaRewardScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	// Holding the opener must not immediately dismiss the new menu. Within the
	// modal, D-pad keys belong to navigation; the gamepad Back action closes it.
	if (!Event.IsRepeat() && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || (!Key.IsGamepadKey() && HUD && HUD->IsRewardToggleKey(Key))))
	{
		CloseSelection();
		return FReply::Handled();
	}
	if (!Event.IsRepeat())
	{
		if (Key == EKeys::One) { SelectCard(0); return FReply::Handled(); }
		if (Key == EKeys::Two) { SelectCard(1); return FReply::Handled(); }
		if (Key == EKeys::Three) { SelectCard(2); return FReply::Handled(); }
		if (Key == EKeys::Enter) { ConfirmSelection(); return FReply::Handled(); }
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

#undef LOCTEXT_NAMESPACE
