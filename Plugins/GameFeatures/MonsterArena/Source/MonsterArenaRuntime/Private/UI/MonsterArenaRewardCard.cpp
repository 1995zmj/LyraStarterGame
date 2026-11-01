#include "UI/MonsterArenaRewardCard.h"

#include "UI/MonsterArenaRewardStat.h"
#include "RunProgressionComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Player/LyraPlayerState.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewardCard"

void UMonsterArenaRewardCard::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SelectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSelected);
}

void UMonsterArenaRewardCard::Configure(const FRunRewardOption& Card, int32 InChoiceIndex, int32 StackCount, URunProgressionComponent* Progression)
{
	ChoiceIndex = InChoiceIndex;
	DisplayedCardId = Card.CardId;
	ChoiceNumber->SetText(FText::AsNumber(ChoiceIndex + 1));
	CardTitle->SetText(Card.Title);
	CardDescription->SetText(Card.Description);
	CardStack->SetText(FText::Format(LOCTEXT("Stacks", "本局已选 {0} 次"), FText::AsNumber(StackCount)));
	EffectsBox->ClearChildren();
	EffectRows.Reset();
	UClass* RowClass = EffectRowClass.LoadSynchronous();
	if (!RowClass) { return; }
	const ALyraPlayerState* Player = Progression ? Cast<ALyraPlayerState>(Progression->GetOwner()) : nullptr;
	const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	TSet<ERunRewardBehavior> Seen;
	for (const FRunRewardAction& Action : Card.Actions)
	{
		if (Seen.Contains(Action.Behavior)) { continue; }
		Seen.Add(Action.Behavior);
		const float Value = Card.GetMagnitude(Action.Behavior);
		FText Preview;
		if (ASC && Action.Behavior == ERunRewardBehavior::DamageBonus)
		{
			const float Damage = ASC->GetNumericAttribute(ULyraCombatSet::GetBaseDamageAttribute());
			Preview = FText::Format(LOCTEXT("DamagePreview", "{0} → {1}"), FText::AsNumber(Damage), FText::AsNumber(Damage + Value));
		}
		else if (ASC && Action.Behavior == ERunRewardBehavior::MaxHealthBonus)
		{
			const float Health = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
			Preview = FText::Format(LOCTEXT("HealthPreview", "{0} → {1}"), FText::AsNumber(Health), FText::AsNumber(Health + Value));
		}
		else if (Progression && Action.Behavior == ERunRewardBehavior::DamagePerKill)
		{
			const float Rate = Progression->GetDamagePerKill();
			Preview = FText::Format(LOCTEXT("KillPreview", "{0} → {1}；本局已累积 +{2}"), FText::AsNumber(Rate), FText::AsNumber(Rate + Value), FText::AsNumber(Progression->State.AccumulatedKillDamage));
		}
		if (UMonsterArenaRewardStat* Row = CreateWidget<UMonsterArenaRewardStat>(this, RowClass))
		{
			EffectsBox->AddChildToVerticalBox(Row);
			Row->Configure(Action.Behavior, Value, Preview);
			EffectRows.Add(Row);
		}
	}
}

void UMonsterArenaRewardCard::SetSelected(bool bSelected)
{
	CardBorder->SetBrushColor(bSelected ? SelectedColor : NormalColor);
}

void UMonsterArenaRewardCard::HandleSelected()
{
	if (GetIsEnabled() && ChoiceIndex != INDEX_NONE) { OnSelected.Broadcast(ChoiceIndex); }
}

#undef LOCTEXT_NAMESPACE
