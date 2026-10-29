#include "ArenaMeleeDamageEffect.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_MonsterArena_MeleeDamage, "MonsterArena.Damage.Melee");

UArenaMeleeDamageEffect::UArenaMeleeDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = ULyraHealthSet::GetDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = TAG_MonsterArena_MeleeDamage;
	Modifier.ModifierMagnitude = Magnitude;
}
