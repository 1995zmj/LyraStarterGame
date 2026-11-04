#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "NativeGameplayTags.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "Character/LyraHealthComponent.h"
#include "ArenaMonsterConfig.h"
#include "ArenaMonster.generated.h"

class ULyraAbilitySystemComponent;
class ULyraHealthComponent;
class ULyraHealthSet;
class ULyraCombatSet;
class UStateTree;
class UStateTreeComponent;
class AArenaDefenseCore;
class UArenaWaveDirectorComponent;
class UStaticMeshComponent;
struct FGameplayEffectSpec;

/** Death event observed by the server-side kill-credit handler; monsters never award rewards themselves. */
MONSTERARENARUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MonsterArena_Elimination_Message);

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

	/**
	 * Spawn-time type defaults only. The wave director writes this once before FinishSpawning and
	 * gameplay never mutates it. Runtime state is separate: current and max health live in the
	 * replicated ULyraHealthSet (Health/MaxHealth) and are exposed by GetHealth()/GetMaxHealth().
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MonsterConfig, Category = "Monster")
	FArenaMonsterConfig MonsterConfig;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI")
	TSoftObjectPtr<UStateTree> StateTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/MonsterArena/AI/ST_ArenaMonster.ST_ArenaMonster")));
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Monster|AI")
	TWeakObjectPtr<APawn> AITarget;

	/** Applies the wave-selected monster type. Server-only; call before FinishSpawning. */
	void ApplyConfig(const FArenaMonsterConfig& Config);
	/** Runtime current health, read from the replicated health set (not from MonsterConfig). */
	UFUNCTION(BlueprintPure, Category = "Monster|Health")
	float GetHealth() const;
	/** Runtime maximum health, read from the replicated health set (not from MonsterConfig). */
	UFUNCTION(BlueprintPure, Category = "Monster|Health")
	float GetMaxHealth() const;
	bool IsAliveForAI() const;
	bool IsValidAITarget(const APawn* Target) const;
	bool IsTargetInMeleeRange() const;
	void TryMeleeAttack();
	void UpdateNavigation(float DeltaTime);
	APawn* ResolveWaveTarget(APawn* Candidate);
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<AArenaDefenseCore> DefenseTarget;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Monster|AI")
	bool bHunter = true;
	TWeakObjectPtr<UArenaWaveDirectorComponent> WaveDirector;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	TObjectPtr<ULyraHealthComponent> HealthComponent;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystem;
	UPROPERTY()
	TObjectPtr<ULyraHealthSet> HealthSet;
	UPROPERTY()
	TObjectPtr<ULyraCombatSet> CombatSet;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;
	UFUNCTION()
	void OnRep_MonsterConfig();
	void ApplyBodyScale();
	bool bDeathSettled = false;
	double NextAttackTime = 0.0;
	float PathRemaining = 0.0f;
	float StuckTime = 0.0f;
	FVector LastNavigationPosition = FVector::ZeroVector;
	TArray<FVector> PathPoints;
	int32 PathIndex = 0;
	double TargetHoldUntil = 0;
	double IgnorePlayersUntil = 0;
	void HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
};
