#include "RunRewardCardDefinition.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewards"

URunRewardPoolDefinition::URunRewardPoolDefinition()
{
	auto Add = [this](FName Id, const FText& Title, const FText& Description, float Health, float Damage, float Heal, float HealFraction, int32 Ammo)
	{
		FRunRewardOption& Card = Cards.AddDefaulted_GetRef().Option;
		Card.CardId = Id;
		Card.Title = Title;
		Card.Description = Description;
		Card.MaxHealthBonus = Health;
		Card.DamageBonus = Damage;
		Card.HealAmount = Heal;
		Card.HealFraction = HealFraction;
		Card.SpareAmmo = Ammo;
	};
	Add(TEXT("Firepower"), LOCTEXT("FirepowerTitle", "火力强化"), LOCTEXT("FirepowerDescription", "本局基础伤害 +3"), 0, 3, 0, 0, 0);
	Add(TEXT("Vitality"), LOCTEXT("VitalityTitle", "体魄强化"), LOCTEXT("VitalityDescription", "本局最大生命 +20\n立即恢复 20 生命"), 20, 0, 20, 0, 0);
	Add(TEXT("Balance"), LOCTEXT("BalanceTitle", "均衡成长"), LOCTEXT("BalanceDescription", "本局基础伤害 +1、最大生命 +10\n立即恢复 10 生命"), 10, 1, 10, 0, 0);
	Add(TEXT("FieldMedicine"), LOCTEXT("MedicineTitle", "战地医疗"), LOCTEXT("MedicineDescription", "立即恢复最大生命值的 40%"), 0, 0, 0, 0.4f, 0);
	Add(TEXT("AmmoSupply"), LOCTEXT("AmmoTitle", "弹药补给"), LOCTEXT("AmmoDescription", "步枪备用弹药 +60\n仍需正常换弹"), 0, 0, 0, 0, 60);
}

#undef LOCTEXT_NAMESPACE
