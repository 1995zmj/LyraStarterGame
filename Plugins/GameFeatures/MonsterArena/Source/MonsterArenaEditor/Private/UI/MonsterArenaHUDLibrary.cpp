#include "UI/MonsterArenaHUDLibrary.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/MonsterArenaHUD.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

UBlueprint* UMonsterArenaHUDLibrary::CreateHUD()
{
	const TCHAR* Path = TEXT("/MonsterArena/UI/W_MonsterArenaHUD.W_MonsterArenaHUD");
	if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		return Cast<UWidgetBlueprint>(Existing);
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
	if (!Cast<UTextBlock>(Tree->FindWidget(TEXT("Status"))) || !Cast<UProgressBar>(Tree->FindWidget(TEXT("ExperienceBar")))
		|| !Cast<UTextBlock>(Tree->FindWidget(TEXT("Reward"))))
	{
		return false;
	}
	FKismetEditorUtilities::CompileBlueprint(Widget);
	return Widget->Status != BS_Error && Widget->GeneratedClass && Widget->GeneratedClass->IsChildOf(UMonsterArenaHUD::StaticClass());
}
