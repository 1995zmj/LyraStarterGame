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
	virtual void Tick(float DeltaSeconds) override;
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
	float AttackCooldown = 0.0f;
	void HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
};
