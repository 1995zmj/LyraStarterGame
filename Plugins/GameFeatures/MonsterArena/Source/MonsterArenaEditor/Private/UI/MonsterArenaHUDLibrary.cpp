#include "UI/MonsterArenaHUDLibrary.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/NamedSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/MonsterArenaCoreHealthWidget.h"
#include "UI/MonsterArenaHUD.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace
{
	/** Adds the designer-visible mount point for the separate core-health widget. */
	UNamedSlot* AddCoreHealthSlot(UWidgetBlueprint* View)
	{
		UWidgetTree* Tree = View ? View->WidgetTree : nullptr;
		UCanvasPanel* Root = Tree ? Cast<UCanvasPanel>(Tree->RootWidget) : nullptr;
		if (!Root) { return nullptr; }
		if (UNamedSlot* Existing = Cast<UNamedSlot>(Tree->FindWidget(TEXT("CoreHealthSlot")))) { return Existing; }
		UNamedSlot* Slot = Tree->ConstructWidget<UNamedSlot>(UNamedSlot::StaticClass(), TEXT("CoreHealthSlot"));
		UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Slot);
		CanvasSlot->SetPosition(FVector2D(32, 40));
		CanvasSlot->SetSize(FVector2D(600, 70));
		return Slot;
	}

	/** Places the health widget in the designer tree; runtime C++ only binds it. */
	UMonsterArenaCoreHealthWidget* AddCoreHealthChild(UWidgetBlueprint* View, UNamedSlot* Slot)
	{
		UWidgetTree* Tree = View ? View->WidgetTree : nullptr;
		if (!Tree || !Slot) { return nullptr; }
		if (UMonsterArenaCoreHealthWidget* Existing = Cast<UMonsterArenaCoreHealthWidget>(Tree->FindWidget(TEXT("CoreHealthDisplay"))))
		{
			if (Slot->GetContent() != Existing) { Slot->SetContent(Existing); }
			return Existing;
		}
		UClass* HealthClass = LoadClass<UMonsterArenaCoreHealthWidget>(nullptr, TEXT("/MonsterArena/UI/W_MonsterArenaCoreHealth.W_MonsterArenaCoreHealth_C"));
		if (!HealthClass) { return nullptr; }
		UMonsterArenaCoreHealthWidget* Health = Tree->ConstructWidget<UMonsterArenaCoreHealthWidget>(HealthClass, TEXT("CoreHealthDisplay"));
		Health->bIsVariable = true;
		Slot->SetContent(Health);
		return Health;
	}
}

UBlueprint* UMonsterArenaHUDLibrary::CreateHUD()
{
	const TCHAR* Path = TEXT("/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD");
	if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet | LOAD_DisableCompileOnLoad))
	{
		UWidgetBlueprint* View = Cast<UWidgetBlueprint>(Existing);
		if (!View || !View->WidgetTree || !Cast<UCanvasPanel>(View->WidgetTree->RootWidget)) { return nullptr; }
		bool bChanged = false;
		UNamedSlot* Slot = Cast<UNamedSlot>(View->WidgetTree->FindWidget(TEXT("CoreHealthSlot")));
		if (!Slot)
		{
			View->Modify();
			Slot = AddCoreHealthSlot(View);
			if (!Slot) { return nullptr; }
			bChanged = true;
		}
		if (!View->WidgetTree->FindWidget(TEXT("CoreHealthDisplay")))
		{
			View->Modify();
			if (!AddCoreHealthChild(View, Slot)) { return nullptr; }
			bChanged = true;
		}
		if (bChanged)
		{
			// New widgets need a variable GUID before the structural change triggers a compile.
			auto RegisterGuid = [View](UWidget* Widget)
			{
				if (Widget && !View->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) { View->OnVariableAdded(Widget->GetFName()); }
			};
			RegisterGuid(Slot);
			RegisterGuid(Cast<UWidget>(View->WidgetTree->FindWidget(TEXT("CoreHealthDisplay"))));
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(View);
		}
		return View;
	}
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UUserWidget::StaticClass();
	IAssetTools& Assets = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Assets.CreateAsset(TEXT("W_MonsterArenaHUD"), TEXT("/MonsterArena/UI"), UWidgetBlueprint::StaticClass(), Factory));
	if (!Blueprint)
	{
		return nullptr;
	}
	UWidgetTree* Tree = Blueprint->WidgetTree;
	UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
	Tree->RootWidget = Root;
	UVerticalBox* Panel = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ProgressionPanel"));
	UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Panel);
	Slot->SetPosition(FVector2D(32, 120));
	Slot->SetSize(FVector2D(600, 180));
	UNamedSlot* CoreHealthSlot = AddCoreHealthSlot(Blueprint);
	if (!CoreHealthSlot || !AddCoreHealthChild(Blueprint, CoreHealthSlot)) { return nullptr; }
	UTextBlock* Status = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Status"));
	UProgressBar* Experience = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ExperienceBar"));
	UTextBlock* Reward = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Reward"));
	Status->bIsVariable = Experience->bIsVariable = Reward->bIsVariable = true;
	Status->SetText(FText::FromString(TEXT("等级 1  |  经验 0/60  |  击杀 0")));
	Status->SetAutoWrapText(true);
	Experience->SetFillColorAndOpacity(FLinearColor(0.1f, 0.8f, 0.6f));
	Reward->SetText(FText::FromString(TEXT("击杀怪物获取经验，升级后可选择强化")));
	Reward->SetAutoWrapText(true);
	Reward->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 1.0f, 0.75f)));
	Panel->AddChildToVerticalBox(Status);
	Panel->AddChildToVerticalBox(Experience);
	Panel->AddChildToVerticalBox(Reward);
	Blueprint->ParentClass = UMonsterArenaHUD::StaticClass();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return Blueprint;
}

bool UMonsterArenaHUDLibrary::ValidateHUD(UBlueprint* Blueprint)
{
	UWidgetBlueprint* Widget = Cast<UWidgetBlueprint>(Blueprint);
	if (!Widget || !Widget->ParentClass || !Widget->ParentClass->IsChildOf(UMonsterArenaHUD::StaticClass()) || !Widget->WidgetTree)
	{
		return false;
	}
	UWidgetTree* Tree = Widget->WidgetTree;
	UNamedSlot* Slot = Cast<UNamedSlot>(Tree->FindWidget(TEXT("CoreHealthSlot")));
	UMonsterArenaCoreHealthWidget* Health = Cast<UMonsterArenaCoreHealthWidget>(Tree->FindWidget(TEXT("CoreHealthDisplay")));
	if (!Cast<UTextBlock>(Tree->FindWidget(TEXT("Status"))) || !Cast<UProgressBar>(Tree->FindWidget(TEXT("ExperienceBar")))
		|| !Cast<UTextBlock>(Tree->FindWidget(TEXT("Reward"))) || !Slot || !Health || Health->GetParent() != Slot)
	{
		return false;
	}
	FKismetEditorUtilities::CompileBlueprint(Widget);
	return Widget->Status != BS_Error && Widget->GeneratedClass && Widget->GeneratedClass->IsChildOf(UMonsterArenaHUD::StaticClass());
}
