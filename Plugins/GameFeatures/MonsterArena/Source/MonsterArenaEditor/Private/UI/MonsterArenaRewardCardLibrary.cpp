#include "UI/MonsterArenaRewardCardLibrary.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/MonsterArenaRewardCard.h"
#include "UI/MonsterArenaRewardStat.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace
{
	UWidgetBlueprint* Existing(const TCHAR* Name)
	{
		return Cast<UWidgetBlueprint>(StaticLoadObject(UObject::StaticClass(), nullptr,
			*FString::Printf(TEXT("/MonsterArena/UI/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet));
	}
	UWidgetBlueprint* NewView(const TCHAR* Name)
	{
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = UUserWidget::StaticClass();
		return Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get()
			.CreateAsset(Name, TEXT("/MonsterArena/UI"), UWidgetBlueprint::StaticClass(), Factory));
	}
	UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Value, int32 Size)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Widget->bIsVariable = true;
		Widget->SetText(FText::FromString(Value));
		Widget->SetAutoWrapText(true);
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Widget->SetFont(Font);
		return Widget;
	}
	bool Has(UWidgetBlueprint* View, const TCHAR* Name, UClass* Class)
	{
		UWidget* Widget = View->WidgetTree->FindWidget(Name);
		return Widget && Widget->IsA(Class);
	}
	bool Compile(UWidgetBlueprint* View, UClass* Parent)
	{
		FKismetEditorUtilities::CompileBlueprint(View);
		return View->Status != BS_Error && View->GeneratedClass && View->GeneratedClass->IsChildOf(Parent);
	}
}

UBlueprint* UMonsterArenaRewardCardLibrary::CreateStat()
{
	if (UWidgetBlueprint* View = Existing(TEXT("W_MonsterArenaRewardStat"))) { return View; }
	UWidgetBlueprint* View = NewView(TEXT("W_MonsterArenaRewardStat"));
	if (!View) { return nullptr; }
	UWidgetTree* Tree = View->WidgetTree;
	UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>();
	Tree->RootWidget = Root;
	UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>();
	Root->AddChildToVerticalBox(Line);
	Line->AddChildToHorizontalBox(Text(Tree, TEXT("EffectName"), TEXT("效果名称"), 16))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UTextBlock* Value = Text(Tree, TEXT("EffectValue"), TEXT("+1"), 16);
	Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.95f, 0.8f)));
	Line->AddChildToHorizontalBox(Value)->SetPadding(FMargin(8, 0, 0, 0));
	UTextBlock* Preview = Text(Tree, TEXT("EffectPreview"), TEXT("当前值 → 强化后"), 14);
	Preview->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.95f, 0.8f)));
	Root->AddChildToVerticalBox(Preview)->SetPadding(FMargin(0, 2, 0, 10));
	View->ParentClass = UMonsterArenaRewardStat::StaticClass();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(View);
	return View;
}

UBlueprint* UMonsterArenaRewardCardLibrary::CreateCard()
{
	if (UWidgetBlueprint* View = Existing(TEXT("W_MonsterArenaRewardCard"))) { return View; }
	UClass* StatClass = LoadClass<UMonsterArenaRewardStat>(nullptr, TEXT("/MonsterArena/UI/W_MonsterArenaRewardStat.W_MonsterArenaRewardStat_C"));
	if (!StatClass) { return nullptr; }
	UWidgetBlueprint* View = NewView(TEXT("W_MonsterArenaRewardCard"));
	if (!View) { return nullptr; }
	UWidgetTree* Tree = View->WidgetTree;
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectButton"));
	Button->bIsVariable = true;
	Tree->RootWidget = Button;
	UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
	Border->bIsVariable = true;
	Border->SetPadding(FMargin(22));
	Button->AddChild(Border);
	USizeBox* Size = Tree->ConstructWidget<USizeBox>();
	Size->SetMinDesiredHeight(280);
	Border->AddChild(Size);
	UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>();
	Size->AddChild(Column);
	UTextBlock* Number = Text(Tree, TEXT("ChoiceNumber"), TEXT("1"), 30);
	Number->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.85f, 0.75f)));
	Column->AddChildToVerticalBox(Number);
	Column->AddChildToVerticalBox(Text(Tree, TEXT("CardTitle"), TEXT("卡牌名称"), 23))->SetPadding(FMargin(0, 16, 0, 14));
	Column->AddChildToVerticalBox(Text(Tree, TEXT("CardDescription"), TEXT("卡牌描述"), 16));
	UVerticalBox* Effects = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EffectsBox"));
	Effects->bIsVariable = true;
	Column->AddChildToVerticalBox(Effects)->SetPadding(FMargin(0, 16, 0, 8));
	// A sample child makes the composition visible in the designer. Configure replaces it.
	Effects->AddChildToVerticalBox(Tree->ConstructWidget<UMonsterArenaRewardStat>(StatClass, TEXT("SampleEffect")));
	Column->AddChildToVerticalBox(Text(Tree, TEXT("CardStack"), TEXT("本局已选 0 次"), 14));
	View->ParentClass = UMonsterArenaRewardCard::StaticClass();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(View);
	return View;
}

bool UMonsterArenaRewardCardLibrary::ValidateStat(UBlueprint* Blueprint)
{
	UWidgetBlueprint* View = Cast<UWidgetBlueprint>(Blueprint);
	if (!View || !View->WidgetTree || !View->ParentClass || !View->ParentClass->IsChildOf(UMonsterArenaRewardStat::StaticClass())) { return false; }
	for (const TCHAR* Name : {TEXT("EffectName"), TEXT("EffectValue"), TEXT("EffectPreview")})
	{
		if (!Has(View, Name, UTextBlock::StaticClass())) { return false; }
	}
	return Compile(View, UMonsterArenaRewardStat::StaticClass());
}

bool UMonsterArenaRewardCardLibrary::ValidateCard(UBlueprint* Blueprint)
{
	UWidgetBlueprint* View = Cast<UWidgetBlueprint>(Blueprint);
	if (!View || !View->WidgetTree || !View->ParentClass || !View->ParentClass->IsChildOf(UMonsterArenaRewardCard::StaticClass())) { return false; }
	for (const TCHAR* Name : {TEXT("ChoiceNumber"), TEXT("CardTitle"), TEXT("CardDescription"), TEXT("CardStack")})
	{
		if (!Has(View, Name, UTextBlock::StaticClass())) { return false; }
	}
	return Has(View, TEXT("SelectButton"), UButton::StaticClass()) && Has(View, TEXT("CardBorder"), UBorder::StaticClass())
		&& Has(View, TEXT("EffectsBox"), UVerticalBox::StaticClass()) && Compile(View, UMonsterArenaRewardCard::StaticClass());
}
