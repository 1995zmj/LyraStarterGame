#include "UI/MonsterArenaRewardScreenLibrary.h"

#include "AssetToolsModule.h"
#include "Animation/WidgetAnimation.h"
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
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/MonsterArenaRewardScreen.h"
#include "UI/MonsterArenaRewardCard.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

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

namespace
{
	void SyncWidgetGuids(UWidgetBlueprint* View)
	{
		TSet<FName> Names;
		View->ForEachSourceWidget([&](UWidget* Widget)
		{
			Names.Add(Widget->GetFName());
			if (!View->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				View->OnVariableAdded(Widget->GetFName());
			}
		});
		for (UWidgetAnimation* Animation : View->Animations)
		{
			if (Animation) { Names.Add(Animation->GetFName()); }
		}
		TArray<FName> Existing;
		View->WidgetVariableNameToGuidMap.GetKeys(Existing);
		for (FName Name : Existing)
		{
			if (!Names.Contains(Name)) { View->OnVariableRemoved(Name); }
		}
	}

	bool MigrateCards(UWidgetBlueprint* View, UClass* CardClass)
	{
		UWidgetTree* Tree = View ? View->WidgetTree : nullptr;
		if (!Tree) { return false; }
		if (Tree->FindWidget(TEXT("Card1")))
		{
			for (int32 Index = 1; Index <= 3; ++Index)
			{
				if (!Cast<UMonsterArenaRewardCard>(Tree->FindWidget(*FString::Printf(TEXT("Card%d"), Index)))) { return false; }
			}
			return true;
		}
		// Preflight every legacy slot before touching the saved layout.
		for (int32 Index = 1; Index <= 3; ++Index)
		{
			UWidget* Old = Tree->FindWidget(*FString::Printf(TEXT("CardButton%d"), Index));
			if (!Old || !Cast<UHorizontalBox>(Old->GetParent()) || !Cast<UHorizontalBoxSlot>(Old->Slot)) { return false; }
		}
		View->Modify();
		for (int32 Index = 1; Index <= 3; ++Index)
		{
			UWidget* Old = Tree->FindWidget(*FString::Printf(TEXT("CardButton%d"), Index));
			UHorizontalBox* Parent = CastChecked<UHorizontalBox>(Old->GetParent());
			UHorizontalBoxSlot* OldSlot = CastChecked<UHorizontalBoxSlot>(Old->Slot);
			const int32 ChildIndex = Parent->GetChildIndex(Old);
			const FMargin Padding = OldSlot->GetPadding();
			const FSlateChildSize Size = OldSlot->GetSize();
			Parent->RemoveChild(Old);
			UMonsterArenaRewardCard* Card = Tree->ConstructWidget<UMonsterArenaRewardCard>(CardClass, *FString::Printf(TEXT("Card%d"), Index));
			Card->bIsVariable = true;
			UHorizontalBoxSlot* Slot = CastChecked<UHorizontalBoxSlot>(Parent->InsertChildAt(ChildIndex, Card));
			Slot->SetPadding(Padding);
			Slot->SetSize(Size);
		}
		SyncWidgetGuids(View);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(View);
		return true;
	}
}

UBlueprint* UMonsterArenaRewardScreenLibrary::CreateRewardScreen()
{
	UClass* CardClass = LoadClass<UMonsterArenaRewardCard>(nullptr, TEXT("/MonsterArena/UI/W_MonsterArenaRewardCard.W_MonsterArenaRewardCard_C"));
	if (!CardClass) { return nullptr; }
	if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/MonsterArena/UI/W_MonsterArenaRewardScreen.W_MonsterArenaRewardScreen"), nullptr, LOAD_NoWarn | LOAD_Quiet | LOAD_DisableCompileOnLoad))
	{
		UWidgetBlueprint* View = Cast<UWidgetBlueprint>(Existing);
		return MigrateCards(View, CardClass) ? View : nullptr;
	}
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UUserWidget::StaticClass();
	IAssetTools& Assets = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Assets.CreateAsset(TEXT("W_MonsterArenaRewardScreen"), TEXT("/MonsterArena/UI"), UWidgetBlueprint::StaticClass(), Factory));
	if (!Blueprint)
	{
		return nullptr;
	}
	UWidgetTree* WidgetTree = Blueprint->WidgetTree;
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
	UTextBlock* PendingText = MakeText(WidgetTree, FText::GetEmpty(), 18);
	PendingText->Rename(TEXT("PendingText"));
	PendingText->bIsVariable = true;
	Column->AddChildToVerticalBox(PendingText)->SetPadding(FMargin(0, 0, 0, 24));
	UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Cards);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UMonsterArenaRewardCard* Card = WidgetTree->ConstructWidget<UMonsterArenaRewardCard>(CardClass, *FString::Printf(TEXT("Card%d"), Index + 1));
		Card->bIsVariable = true;
		UHorizontalBoxSlot* Slot = Cards->AddChildToHorizontalBox(Card);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(FMargin(Index ? 8.0f : 0.0f, 0, Index < 2 ? 8.0f : 0.0f, 0));
	}
	UTextBlock* FeedbackText = MakeText(WidgetTree, LOCTEXT("ChooseHint", "选择一张卡牌，然后确认。"), 16, FLinearColor(0.9f, 0.8f, 0.5f));
	FeedbackText->Rename(TEXT("FeedbackText"));
	FeedbackText->bIsVariable = true;
	Column->AddChildToVerticalBox(FeedbackText)->SetPadding(FMargin(0, 20, 0, 12));
	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Actions);
	UButton* ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
	ConfirmButton->bIsVariable = true;
	ConfirmButton->AddChild(MakeText(WidgetTree, LOCTEXT("Confirm", "  确认强化  "), 19));
	Actions->AddChildToHorizontalBox(ConfirmButton)->SetPadding(FMargin(0, 0, 18, 0));
	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->bIsVariable = true;
	CloseButton->AddChild(MakeText(WidgetTree, LOCTEXT("Close", "  返回战斗 · 保留机会  "), 19));
	Actions->AddChildToHorizontalBox(CloseButton);
	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("Danger", "战斗仍在继续，你仍会受到伤害。可随时返回，候选与次数都会保留。"), 14, FLinearColor(1.0f, 0.65f, 0.5f)))->SetPadding(FMargin(0, 20, 0, 0));
	Blueprint->ParentClass = UMonsterArenaRewardScreen::StaticClass();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return Blueprint;
}

bool UMonsterArenaRewardScreenLibrary::ValidateRewardScreen(UBlueprint* Blueprint)
{
	UWidgetBlueprint* Widget = Cast<UWidgetBlueprint>(Blueprint);
	if (!Widget || !Widget->ParentClass || !Widget->ParentClass->IsChildOf(UMonsterArenaRewardScreen::StaticClass()) || !Widget->WidgetTree)
	{
		return false;
	}
	UWidgetTree* Tree = Widget->WidgetTree;
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		if (!Cast<UMonsterArenaRewardCard>(Tree->FindWidget(*FString::Printf(TEXT("Card%d"), Index)))) { return false; }
	}
	if (!Cast<UTextBlock>(Tree->FindWidget(TEXT("PendingText"))) || !Cast<UTextBlock>(Tree->FindWidget(TEXT("FeedbackText")))
		|| !Cast<UButton>(Tree->FindWidget(TEXT("ConfirmButton"))) || !Cast<UButton>(Tree->FindWidget(TEXT("CloseButton"))))
	{
		return false;
	}
	FKismetEditorUtilities::CompileBlueprint(Widget);
	return Widget->Status != BS_Error && Widget->GeneratedClass && Widget->GeneratedClass->IsChildOf(UMonsterArenaRewardScreen::StaticClass());
}

#undef LOCTEXT_NAMESPACE
