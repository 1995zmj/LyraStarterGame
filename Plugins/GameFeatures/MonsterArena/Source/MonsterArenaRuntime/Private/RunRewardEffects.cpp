#include "RunRewardEffects.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RunRewardMagnitude, "MonsterArena.Reward.Magnitude");

static void ConfigureReward(UGameplayEffect* Effect, FGameplayAttribute Attribute)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = TAG_RunRewardMagnitude;
	Modifier.ModifierMagnitude = Magnitude;
}

URunHealthRewardEffect::URunHealthRewardEffect()
{
	ConfigureReward(this, ULyraHealthSet::GetMaxHealthAttribute());
}

URunDamageRewardEffect::URunDamageRewardEffect()
{
	ConfigureReward(this, ULyraCombatSet::GetBaseDamageAttribute());
}
