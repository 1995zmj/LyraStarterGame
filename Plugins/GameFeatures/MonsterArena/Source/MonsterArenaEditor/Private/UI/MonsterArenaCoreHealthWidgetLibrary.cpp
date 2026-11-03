#include "UI/MonsterArenaCoreHealthWidgetLibrary.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/MonsterArenaCoreHealthWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

#define LOCTEXT_NAMESPACE "MonsterArenaCoreHealth"

UBlueprint* UMonsterArenaCoreHealthWidgetLibrary::CreateCoreHealthWidget()
{
	const TCHAR* Path = TEXT("/MonsterArena/UI/W_MonsterArenaCoreHealth.W_MonsterArenaCoreHealth");
	if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet | LOAD_DisableCompileOnLoad))
	{
		return Cast<UWidgetBlueprint>(Existing);
	}
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UUserWidget::StaticClass();
	IAssetTools& Assets = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Assets.CreateAsset(TEXT("W_MonsterArenaCoreHealth"), TEXT("/MonsterArena/UI"), UWidgetBlueprint::StaticClass(), Factory));
	if (!Blueprint)
	{
		return nullptr;
	}
	UWidgetTree* Tree = Blueprint->WidgetTree;
	UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CoreHealthRoot"));
	Tree->RootWidget = Root;
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
	Text->bIsVariable = true;
	Text->SetText(LOCTEXT("Health", "大本营血量：1,000 / 1,000"));
	Text->SetAutoWrapText(true);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 20;
	Text->SetFont(Font);
	UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	Bar->bIsVariable = true;
	Bar->SetPercent(1.0f);
	Bar->SetFillColorAndOpacity(FLinearColor(0.85f, 0.25f, 0.2f));
	Root->AddChildToVerticalBox(Text);
	Root->AddChildToVerticalBox(Bar);
	Blueprint->ParentClass = UMonsterArenaCoreHealthWidget::StaticClass();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return Blueprint;
}

bool UMonsterArenaCoreHealthWidgetLibrary::ValidateCoreHealthWidget(UBlueprint* Blueprint)
{
	UWidgetBlueprint* Widget = Cast<UWidgetBlueprint>(Blueprint);
	if (!Widget || !Widget->ParentClass || !Widget->ParentClass->IsChildOf(UMonsterArenaCoreHealthWidget::StaticClass()) || !Widget->WidgetTree)
	{
		return false;
	}
	UWidgetTree* Tree = Widget->WidgetTree;
	if (!Cast<UTextBlock>(Tree->FindWidget(TEXT("HealthText"))) || !Cast<UProgressBar>(Tree->FindWidget(TEXT("HealthBar"))))
	{
		return false;
	}
	FKismetEditorUtilities::CompileBlueprint(Widget);
	return Widget->Status != BS_Error && Widget->GeneratedClass && Widget->GeneratedClass->IsChildOf(UMonsterArenaCoreHealthWidget::StaticClass());
}

#undef LOCTEXT_NAMESPACE
