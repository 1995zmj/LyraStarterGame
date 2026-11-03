#pragma once

#include "GameFramework/Pawn.h"
#include "ArenaDefenseCore.generated.h"

/** Enemy-only defense objective. Player weapon damage does not damage the core. */
UCLASS()
class MONSTERARENARUNTIME_API AArenaDefenseCore : public APawn
{
	GENERATED_BODY()
public:
	AArenaDefenseCore();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly) float MaxHealth = 1000.0f;
	UPROPERTY(Replicated, BlueprintReadOnly) float Health = 1000.0f;
	void ApplyEnemyDamage(float Amount);
	void ResetCore();
};
