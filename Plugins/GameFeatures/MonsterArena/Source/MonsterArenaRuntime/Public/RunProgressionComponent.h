#pragma once

#include "Components/GameFrameworkComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataAsset.h"
#include "RunRewardCardDefinition.h"
#include "RunProgressionComponent.generated.h"

class ULyraAbilitySystemComponent;
class ULyraInventoryItemInstance;

USTRUCT(BlueprintType)
struct FRunRewardGrant
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 OfferId = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 RewardLevel = 0;
	UPROPERTY(BlueprintReadOnly)
	FRunRewardOption Card;
};

/** One owner-only snapshot prevents UI from combining candidates from different offers. */
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
	UPROPERTY(BlueprintReadOnly)
	int32 RunId = 1;
	UPROPERTY(BlueprintReadOnly)
	int32 OfferId = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 PendingChoices = 0;
	UPROPERTY(BlueprintReadOnly)
	bool bRewardMenuOpen = false;
	UPROPERTY(BlueprintReadOnly)
	TArray<FRunRewardOption> Options;
	UPROPERTY(BlueprintReadOnly)
	TArray<FRunRewardGrant> ChosenRewards;
};

UCLASS(BlueprintType)
class MONSTERARENARUNTIME_API URunProgressionDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
	TArray<int32> ExperiencePerLevel = {60, 100, 140, 180, 220, 260, 300, 340, 380};
	/** Leave empty to use MonsterArena's five native cards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
	TObjectPtr<URunRewardPoolDefinition> RewardPool;

	int32 GetExperienceRequired(int32 Level) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRunProgressionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRunRewardRequestFailed, FText, Reason);

/** The server owns XP, deferred choices and the ledger of rewards actually selected. */
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
	UPROPERTY(BlueprintAssignable)
	FRunRewardRequestFailed OnRewardRequestFailed;

	UFUNCTION(BlueprintPure)
	int32 GetExperienceRequired() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddExperience(int32 Amount);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RecordMonsterKill(int32 ExperienceReward);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ResetRun();
	UFUNCTION(BlueprintPure)
	bool CanOpenRewardChoices() const;
	UFUNCTION(BlueprintCallable)
	void RequestOpenRewardChoices();
	UFUNCTION(BlueprintCallable)
	void RequestCloseRewardChoices();
	UFUNCTION(BlueprintCallable)
	void RequestSelectReward(int32 RunId, int32 OfferId, FName CardId);

private:
	UFUNCTION()
	void OnRep_State();
	UFUNCTION(Server, Reliable)
	void ServerSetRewardMenuOpen(int32 RunId, bool bOpen);
	UFUNCTION(Server, Reliable)
	void ServerSelectReward(int32 RunId, int32 OfferId, FName CardId);
	UFUNCTION(Client, Reliable)
	void ClientRewardRequestFailed(const FText& Reason);

	bool IsPlayerReady() const;
	bool IsCardApplicable(const FRunRewardOption& Card) const;
	ULyraInventoryItemInstance* FindRifleItem() const;
	void GenerateOffer();
	void SetRewardMenuOpen(bool bOpen);
	void NotifyStateChanged();
	bool ApplyPersistentReward(const FRunRewardOption& Card, TArray<FActiveGameplayEffectHandle>& OutHandles);
	bool RefreshRewards();
	void RemoveRewards();
	TWeakObjectPtr<ULyraAbilitySystemComponent> RewardASC;
	TWeakObjectPtr<APawn> RewardAvatar;
	TArray<FActiveGameplayEffectHandle> RewardHandles;
	int32 AppliedRewardCount = 0;
	TWeakObjectPtr<ULyraAbilitySystemComponent> MenuBlockedASC;
	TWeakObjectPtr<APawn> MenuAvatar;
	bool bResolvingChoice = false;
};
