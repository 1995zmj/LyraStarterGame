#include "UI/MonsterArenaRewardStat.h"

#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewardStat"

void UMonsterArenaRewardStat::Configure(ERunRewardBehavior InBehavior, float InValue, const FText& Preview)
{
	Behavior = InBehavior;
	Value = InValue;
	FText Label;
	switch (Behavior)
	{
	case ERunRewardBehavior::MaxHealthBonus: Label = LOCTEXT("MaxHealth", "生命上限"); break;
	case ERunRewardBehavior::DamageBonus: Label = LOCTEXT("Damage", "基础伤害"); break;
	case ERunRewardBehavior::HealAmount: Label = LOCTEXT("Heal", "立即治疗"); break;
	case ERunRewardBehavior::HealFraction: Label = LOCTEXT("HealFraction", "按生命上限治疗"); break;
	case ERunRewardBehavior::SpareAmmo: Label = LOCTEXT("Ammo", "备用弹药"); break;
	case ERunRewardBehavior::DamagePerKill: Label = LOCTEXT("PerKill", "每次击杀伤害收益"); break;
	default: Label = LOCTEXT("Unknown", "未知效果"); break;
	}
	EffectName->SetText(Label);
	EffectValue->SetText(Behavior == ERunRewardBehavior::HealFraction
		? FText::Format(LOCTEXT("Percent", "+{0}%"), FText::AsNumber(Value * 100.0f))
		: FText::Format(LOCTEXT("Value", "+{0}"), FText::AsNumber(Value)));
	EffectPreview->SetText(Preview);
	EffectPreview->SetVisibility(Preview.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
