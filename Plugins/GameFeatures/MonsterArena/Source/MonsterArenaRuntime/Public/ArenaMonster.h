#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "Character/LyraHealthComponent.h"
#include "ArenaMonster.generated.h"

class ULyraAbilitySystemComponent;
class ULyraHealthComponent;
class ULyraHealthSet;
class ULyraCombatSet;
class UStateTree;
class UStateTreeComponent;
struct FGameplayEffectSpec;

/** Monsters have no PlayerState and must not publish player-elimination messages. */
UCLASS()
class MONSTERARENARUNTIME_API UArenaMonsterHealthComponent : public ULyraHealthComponent
{
	GENERATED_BODY()
protected:
	virtual void HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue) override;
};

/** Prototype monster: own ASC, server movement, melee and one death settlement. */
UCLASS()
class MONSTERARENARUNTIME_API AArenaMonster : public ACharacter, public IAbilitySystemInterface, public ILyraTeamAgentInterface
{
	GENERATED_BODY()
public:
	AArenaMonster(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(200);
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster", meta = (ClampMin = "1"))
	int32 ExperienceReward = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster", meta = (ClampMin = "1"))
	float InitialHealth = 60.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster", meta = (ClampMin = "0"))
	float MeleeDamage = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0"))
	float DetectionRadius = 4000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "1"))
	float MeleeRange = 140.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI", meta = (ClampMin = "0.1"))
	float AttackInterval = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI")
	TSoftObjectPtr<UStateTree> StateTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/MonsterArena/AI/ST_ArenaMonster.ST_ArenaMonster")));
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Monster|AI")
	TWeakObjectPtr<APawn> AITarget;

	bool IsAliveForAI() const;
	bool IsValidAITarget(const APawn* Target) const;
	bool IsTargetInMeleeRange() const;
	void TryMeleeAttack();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<ULyraHealthComponent> HealthComponent;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystem;
	UPROPERTY()
	TObjectPtr<ULyraHealthSet> HealthSet;
	UPROPERTY()
	TObjectPtr<ULyraCombatSet> CombatSet;
	bool bDeathSettled = false;
	double NextAttackTime = 0.0;
	void HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
};
