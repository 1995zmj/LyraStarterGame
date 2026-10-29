#include "MonsterArenaRewardScreen.h"

#include "MonsterArenaComponent.h"
#include "RunProgressionComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Player/LyraPlayerState.h"
#include "PlayerMappableKeySettings.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewards"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, FLinearColor Color = FLinearColor::White)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(Color);
		Label->SetAutoWrapText(true);
		return Label;
	}
}

UMonsterArenaRewardInputAction::UMonsterArenaRewardInputAction()
{
	ValueType = EInputActionValueType::Boolean;
	PlayerMappableKeySettings = CreateDefaultSubobject<UPlayerMappableKeySettings>(TEXT("RewardKeySettings"));
	PlayerMappableKeySettings->Name = TEXT("MonsterArena.OpenRewards");
	PlayerMappableKeySettings->DisplayName = LOCTEXT("OpenRewardsMapping", "打开强化选择");
	PlayerMappableKeySettings->DisplayCategory = LOCTEXT("InputCategory", "怪物竞技场");
}

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

TSharedRef<SWidget> UMonsterArenaRewardScreen::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	CardButtons.Reset();
	CardBorders.Reset();
	CardTitles.Reset();
	CardDescriptions.Reset();
	CardPreviews.Reset();
	CardStacks.Reset();
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Shade = WidgetTree->ConstructWidget<UBorder>();
	Shade->SetBrushColor(FLinearColor(0.012f, 0.018f, 0.03f, 0.88f));
	Root->AddChildToOverlay(Shade);
	UScaleBox* ResponsiveFrame = WidgetTree->ConstructWidget<UScaleBox>();
	ResponsiveFrame->SetStretch(EStretch::ScaleToFit);
	ResponsiveFrame->SetStretchDirection(EStretchDirection::DownOnly);
	UOverlaySlot* ResponsiveSlot = Root->AddChildToOverlay(ResponsiveFrame);
	ResponsiveSlot->SetHorizontalAlignment(HAlign_Fill);
	ResponsiveSlot->SetVerticalAlignment(VAlign_Fill);
	ResponsiveSlot->SetPadding(FMargin(24.0f));
	USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>();
	Frame->SetWidthOverride(1000.0f);
	UScaleBoxSlot* FrameSlot = CastChecked<UScaleBoxSlot>(ResponsiveFrame->AddChild(Frame));
	FrameSlot->SetHorizontalAlignment(HAlign_Center);
	FrameSlot->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->AddChild(Column);
	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("Eyebrow", "MONSTER ARENA  /  本局强化"), 15, FLinearColor(0.3f, 0.85f, 0.75f)));
	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("Title", "选择你的下一项强化"), 34))->SetPadding(FMargin(0, 12, 0, 8));
	PendingText = MakeText(WidgetTree, FText::GetEmpty(), 18);
	Column->AddChildToVerticalBox(PendingText)->SetPadding(FMargin(0, 0, 0, 24));
	UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Cards);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		UHorizontalBoxSlot* CardSlot = Cards->AddChildToHorizontalBox(Button);
		CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CardSlot->SetPadding(FMargin(Index ? 8.0f : 0.0f, 0, Index < 2 ? 8.0f : 0.0f, 0));
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
		Border->SetPadding(FMargin(22.0f));
		Button->AddChild(Border);
		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
		CardSize->SetMinDesiredHeight(280.0f);
		Border->AddChild(CardSize);
		UVerticalBox* CardColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		CardSize->AddChild(CardColumn);
		CardColumn->AddChildToVerticalBox(MakeText(WidgetTree, FText::AsNumber(Index + 1), 30, FLinearColor(0.3f, 0.85f, 0.75f)));
		UTextBlock* Title = MakeText(WidgetTree, LOCTEXT("LoadingCard", "等待候选…"), 23);
		CardColumn->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 16, 0, 14));
		UTextBlock* Description = MakeText(WidgetTree, FText::GetEmpty(), 16, FLinearColor(0.8f, 0.85f, 0.9f));
		CardColumn->AddChildToVerticalBox(Description);
		UTextBlock* Preview = MakeText(WidgetTree, FText::GetEmpty(), 17, FLinearColor(0.4f, 0.95f, 0.8f));
		CardColumn->AddChildToVerticalBox(Preview)->SetPadding(FMargin(0, 16, 0, 8));
		UTextBlock* Stacks = MakeText(WidgetTree, FText::GetEmpty(), 14, FLinearColor(0.65f, 0.7f, 0.75f));
		CardColumn->AddChildToVerticalBox(Stacks);
		CardButtons.Add(Button);
		CardBorders.Add(Border);
		CardTitles.Add(Title);
		CardDescriptions.Add(Description);
		CardPreviews.Add(Preview);
		CardStacks.Add(Stacks);
	}
	CardButtons[0]->OnClicked.AddDynamic(this, &ThisClass::SelectFirst);
	CardButtons[1]->OnClicked.AddDynamic(this, &ThisClass::SelectSecond);
	CardButtons[2]->OnClicked.AddDynamic(this, &ThisClass::SelectThird);
	FeedbackText = MakeText(WidgetTree, LOCTEXT("ChooseHint", "选择一张卡牌，然后确认。"), 16, FLinearColor(0.9f, 0.8f, 0.5f));
	Column->AddChildToVerticalBox(FeedbackText)->SetPadding(FMargin(0, 20, 0, 12));
	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Actions);
	ConfirmButton = WidgetTree->ConstructWidget<UButton>();
	ConfirmButton->AddChild(MakeText(WidgetTree, LOCTEXT("Confirm", "  确认强化  "), 19));
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::ConfirmSelection);
	Actions->AddChildToHorizontalBox(ConfirmButton)->SetPadding(FMargin(0, 0, 18, 0));
	CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->AddChild(MakeText(WidgetTree, LOCTEXT("Close", "  返回战斗 · 保留机会  "), 19));
	CloseButton->OnClicked.AddDynamic(this, &ThisClass::CloseSelection);
	Actions->AddChildToHorizontalBox(CloseButton);
	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("Danger", "战斗仍在继续，你仍会受到伤害。可随时返回，候选与次数都会保留。"), 14, FLinearColor(1.0f, 0.65f, 0.5f)))->SetPadding(FMargin(0, 20, 0, 0));
	RefreshChoices();
	return Super::RebuildWidget();
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
	const ALyraPlayerState* Player = Cast<ALyraPlayerState>(Progression->GetOwner());
	const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	for (int32 Index = 0; Index < CardButtons.Num(); ++Index)
	{
		const bool bHasCard = State.Options.IsValidIndex(Index);
		CardButtons[Index]->SetVisibility(bHasCard ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		CardButtons[Index]->SetIsEnabled(bHasCard && State.bRewardMenuOpen && !bAwaitingResponse);
		if (!bHasCard)
		{
			continue;
		}
		const FRunRewardOption& Card = State.Options[Index];
		CardTitles[Index]->SetText(Card.Title);
		CardDescriptions[Index]->SetText(Card.Description);
		int32 StackCount = 0;
		for (const FRunRewardGrant& Grant : State.ChosenRewards)
		{
			StackCount += Grant.Card.CardId == Card.CardId ? 1 : 0;
		}
		CardStacks[Index]->SetText(FText::Format(LOCTEXT("Stacks", "本局已选 {0} 次"), FText::AsNumber(StackCount)));
		FString Preview;
		if (ASC && Card.DamageBonus > 0.0f)
		{
			const float Damage = ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute());
			Preview += FText::Format(LOCTEXT("DamagePreview", "伤害加成 {0} → {1}"), FText::AsNumber(Damage), FText::AsNumber(Damage + Card.DamageBonus)).ToString();
		}
		if (ASC && Card.MaxHealthBonus > 0.0f)
		{
			if (!Preview.IsEmpty()) { Preview += TEXT("\n"); }
			const float Health = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
			Preview += FText::Format(LOCTEXT("HealthPreview", "生命上限 {0} → {1}"), FText::AsNumber(Health), FText::AsNumber(Health + Card.MaxHealthBonus)).ToString();
		}
		CardPreviews[Index]->SetText(FText::FromString(Preview));
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

void UMonsterArenaRewardScreen::SelectFirst() { SelectCard(0); }
void UMonsterArenaRewardScreen::SelectSecond() { SelectCard(1); }
void UMonsterArenaRewardScreen::SelectThird() { SelectCard(2); }

void UMonsterArenaRewardScreen::RefreshSelectionStyle()
{
	for (int32 Index = 0; Index < CardBorders.Num(); ++Index)
	{
		CardBorders[Index]->SetBrushColor(Index == SelectedIndex ? FLinearColor(0.045f, 0.25f, 0.23f) : FLinearColor(0.035f, 0.055f, 0.09f));
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
	for (UButton* Button : CardButtons) { Button->SetIsEnabled(false); }
	Progression->RequestSelectReward(DisplayedRunId, DisplayedOfferId, CardId);
}

void UMonsterArenaRewardScreen::CloseSelection() { DeactivateWidget(); }
bool UMonsterArenaRewardScreen::NativeOnHandleBackAction() { CloseSelection(); return true; }

UWidget* UMonsterArenaRewardScreen::NativeGetDesiredFocusTarget() const
{
	for (UButton* Button : CardButtons)
	{
		if (Button && Button->GetVisibility() == ESlateVisibility::Visible && Button->GetIsEnabled()) { return Button; }
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
