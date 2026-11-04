#include "ArenaMonsterConfig.h"

bool FArenaMonsterConfig::IsValid() const
{
	return !MonsterId.IsNone()
		&& FMath::IsFinite(InitialHealth) && InitialHealth >= 1.0f && InitialHealth <= 100000.0f
		&& FMath::IsFinite(MeleeDamage) && MeleeDamage >= 0.0f && MeleeDamage <= 100000.0f
		&& FMath::IsFinite(MoveSpeed) && MoveSpeed >= 1.0f && MoveSpeed <= 2000.0f
		&& ExperienceReward >= 1 && ExperienceReward <= 100000
		&& FMath::IsFinite(DetectionRadius) && DetectionRadius >= 1.0f && DetectionRadius <= 100000.0f
		&& FMath::IsFinite(MeleeRange) && MeleeRange >= 1.0f && MeleeRange <= 100000.0f
		&& FMath::IsFinite(AttackInterval) && AttackInterval >= 0.1f && AttackInterval <= 100.0f
		&& FMath::IsFinite(BodyScale) && BodyScale >= 0.1f && BodyScale <= 10.0f;
}
