#include "RunRewardCardDefinition.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewards"

float FRunRewardOption::GetMagnitude(ERunRewardBehavior Behavior) const
{
	double Total = 0.0;
	for (const FRunRewardAction& Action : Actions)
	{
		if (Action.Behavior == Behavior)
		{
			Total += Action.Value;
		}
	}
	return float(Total);
}

bool FRunRewardOption::IsValid() const
{
	if (CardId.IsNone() || Actions.IsEmpty())
	{
		return false;
	}
	for (const FRunRewardAction& Action : Actions)
	{
		double Total = 0.0;
		for (const FRunRewardAction& Other : Actions)
		{
			if (Other.Behavior == Action.Behavior)
			{
				Total += Other.Value;
			}
		}
		// Keep integer ammo exactly representable in the float configuration.
		if (!FMath::IsFinite(Action.Value) || Action.Value <= 0.0f || Action.Value > 16777216.0f)
		{
			return false;
		}
		switch (Action.Behavior)
		{
		case ERunRewardBehavior::MaxHealthBonus:
		case ERunRewardBehavior::DamageBonus:
		case ERunRewardBehavior::HealAmount:
		case ERunRewardBehavior::DamagePerKill:
			break;
		case ERunRewardBehavior::HealFraction:
			if (Total > 1.0 + KINDA_SMALL_NUMBER)
			{
				return false;
			}
			break;
		case ERunRewardBehavior::SpareAmmo:
			if (FMath::Frac(Action.Value) != 0.0f)
			{
				return false;
			}
			break;
		default:
			return false;
		}
		if (Total > 16777216.0)
		{
			return false;
		}
	}
	return true;
}

URunRewardPoolDefinition::URunRewardPoolDefinition()
{
	auto Add = [this](FName Id, const FText& Title, const FText& Description, TArray<FRunRewardAction> Actions)
	{
		FRunRewardOption& Card = Cards.AddDefaulted_GetRef().Option;
		Card.CardId = Id;
		Card.Title = Title;
		Card.Description = Description;
		Card.Actions = MoveTemp(Actions);
	};
	Add(TEXT("Firepower"), LOCTEXT("FirepowerTitle", "火力强化"), LOCTEXT("FirepowerDescription", "本局基础伤害 +3"), {{ERunRewardBehavior::DamageBonus, 3}});
	Add(TEXT("Vitality"), LOCTEXT("VitalityTitle", "体魄强化"), LOCTEXT("VitalityDescription", "本局最大生命 +20\n立即恢复 20 生命"), {{ERunRewardBehavior::MaxHealthBonus, 20}, {ERunRewardBehavior::HealAmount, 20}});
	Add(TEXT("Balance"), LOCTEXT("BalanceTitle", "均衡成长"), LOCTEXT("BalanceDescription", "本局基础伤害 +1、最大生命 +10\n立即恢复 10 生命"), {{ERunRewardBehavior::MaxHealthBonus, 10}, {ERunRewardBehavior::DamageBonus, 1}, {ERunRewardBehavior::HealAmount, 10}});
	Add(TEXT("FieldMedicine"), LOCTEXT("MedicineTitle", "战地医疗"), LOCTEXT("MedicineDescription", "立即恢复最大生命值的 40%"), {{ERunRewardBehavior::HealFraction, 0.4f}});
	Add(TEXT("AmmoSupply"), LOCTEXT("AmmoTitle", "弹药补给"), LOCTEXT("AmmoDescription", "步枪备用弹药 +60\n仍需正常换弹"), {{ERunRewardBehavior::SpareAmmo, 60}});
	Add(TEXT("FirepowerAccumulation"), LOCTEXT("AccumulationTitle", "火力积累"), LOCTEXT("AccumulationDescription", "每击杀一个怪物或敌方玩家，本局基础伤害 +1"), {{ERunRewardBehavior::DamagePerKill, 1}});
}

#undef LOCTEXT_NAMESPACE
