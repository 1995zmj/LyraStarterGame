#pragma once

#include "CoreMinimal.h"
#include "ArenaMonsterConfig.generated.h"

/**
 * Spawn-time type defaults for the shared AArenaMonster class. This is configuration, not state:
 * it is written once before FinishSpawning and must not be mutated at runtime. Current and max
 * health are runtime state stored in the replicated ULyraHealthSet.
 */
USTRUCT(BlueprintType)
struct MONSTERARENARUNTIME_API FArenaMonsterConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName MonsterId;
	/** Seeds the health set's MaxHealth once at spawn; current health starts at this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1")) float InitialHealth = 60.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float MeleeDamage = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1")) float MoveSpeed = 220.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1")) int32 ExperienceReward = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1")) float DetectionRadius = 4000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1")) float MeleeRange = 140.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float AttackInterval = 1.0f;
	/** Visual-only uniform scale of the prototype body; gameplay capsule size stays unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float BodyScale = 1.0f;

	bool IsValid() const;
};
