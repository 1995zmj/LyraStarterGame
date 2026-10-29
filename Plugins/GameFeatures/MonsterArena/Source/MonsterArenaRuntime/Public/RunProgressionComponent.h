#pragma once

#include "Components/GameFrameworkComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataAsset.h"
#include "RunProgressionComponent.generated.h"

class ULyraAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FRunProgressionState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Level = 1;
	UPROPERTY(BlueprintReadOnly)
	int32 Experience = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 Kills = 0;
};

UCLASS(BlueprintType)
class MONSTERARENARUNTIME_API URunProgressionDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
	TArray<int32> ExperiencePerLevel = {60, 100, 140, 180, 220, 260, 300, 340, 380};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards", meta = (ClampMin = "0"))
	float HealthPerReward = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards", meta = (ClampMin = "0"))
	float DamagePerReward = 3.0f;

	int32 GetExperienceRequired(int32 Level) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRunProgressionChanged);

/** Server-owned run state. Rewards are alternating health/damage upgrades for the prototype. */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class MONSTERARENARUNTIME_API URunProgressionComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()
public:
	URunProgressionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<URunProgressionDefinition> Definition;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Progression")
	FRunProgressionState State;
	UPROPERTY(BlueprintAssignable)
	FRunProgressionChanged OnProgressionChanged;

	UFUNCTION(BlueprintPure)
	int32 GetExperienceRequired() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddExperience(int32 Amount);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RecordMonsterKill(int32 ExperienceReward);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ResetRun();

private:
	UFUNCTION()
	void OnRep_State();
	void RefreshRewards();
	void RemoveRewards();
	TWeakObjectPtr<ULyraAbilitySystemComponent> RewardASC;
	TWeakObjectPtr<APawn> RewardAvatar;
	TArray<FActiveGameplayEffectHandle> RewardHandles;
	int32 AppliedLevel = 1;
};
