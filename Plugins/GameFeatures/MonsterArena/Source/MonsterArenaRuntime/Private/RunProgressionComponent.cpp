#include "RunProgressionComponent.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "GameplayEffect.h"
#include "RunRewardEffects.h"
#include "Net/UnrealNetwork.h"
#include "Player/LyraPlayerState.h"

int32 URunProgressionDefinition::GetExperienceRequired(int32 Level) const
{
	return ExperiencePerLevel.IsValidIndex(Level - 1) ? FMath::Max(1, ExperiencePerLevel[Level - 1]) : 0;
}

URunProgressionComponent::URunProgressionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void URunProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!Definition)
	{
		Definition = NewObject<URunProgressionDefinition>(this);
	}
}

void URunProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(URunProgressionComponent, State, COND_OwnerOnly);
}

int32 URunProgressionComponent::GetExperienceRequired() const
{
	return Definition ? Definition->GetExperienceRequired(State.Level) : GetDefault<URunProgressionDefinition>()->GetExperienceRequired(State.Level);
}

void URunProgressionComponent::AddExperience(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || Amount <= 0 || GetExperienceRequired() == 0)
	{
		return;
	}
	int64 Remaining = int64(State.Experience) + Amount;
	while (GetExperienceRequired() > 0 && Remaining >= GetExperienceRequired())
	{
		Remaining -= GetExperienceRequired();
		++State.Level;
	}
	State.Experience = GetExperienceRequired() > 0 ? int32(Remaining) : 0;
	RefreshRewards();
	OnRep_State();
	GetOwner()->ForceNetUpdate();
}

void URunProgressionComponent::RecordMonsterKill(int32 ExperienceReward)
{
	if (GetOwner()->HasAuthority())
	{
		++State.Kills;
		AddExperience(ExperienceReward);
		OnRep_State();
		GetOwner()->ForceNetUpdate();
	}
}

void URunProgressionComponent::ResetRun()
{
	if (GetOwner()->HasAuthority())
	{
		RemoveRewards();
		State = FRunProgressionState();
		OnRep_State();
		GetOwner()->ForceNetUpdate();
	}
}

void URunProgressionComponent::OnRep_State()
{
	OnProgressionChanged.Broadcast();
}

void URunProgressionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner()->HasAuthority())
	{
		RefreshRewards();
	}
}

void URunProgressionComponent::RefreshRewards()
{
	const ALyraPlayerState* Player = Cast<ALyraPlayerState>(GetOwner());
	ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	if (!ASC || !Player->GetPawn() || ASC->GetAvatarActor() != Player->GetPawn() || !ASC->GetSet<ULyraHealthSet>())
	{
		return;
	}
	if (RewardASC != ASC || RewardAvatar != Player->GetPawn())
	{
		RemoveRewards();
		RewardASC = ASC;
		RewardAvatar = Player->GetPawn();
	}
	const URunProgressionDefinition* Config = Definition ? Definition.Get() : GetDefault<URunProgressionDefinition>();
	while (AppliedLevel < State.Level)
	{
		const int32 NewLevel = AppliedLevel + 1;
		const bool bHealth = NewLevel % 2 == 0;
		UClass* EffectClass = bHealth ? URunHealthRewardEffect::StaticClass() : URunDamageRewardEffect::StaticClass();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ASC->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("MonsterArena.Reward.Magnitude")), bHealth ? Config->HealthPerReward : Config->DamagePerReward);
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		if (!Handle.IsValid())
		{
			break;
		}
		RewardHandles.Add(Handle);
		AppliedLevel = NewLevel;
		if (bHealth && ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()) > 0.0f)
		{
			const float Health = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
			const float MaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
			ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), FMath::Min(Health + Config->HealthPerReward, MaxHealth));
		}
	}
}

void URunProgressionComponent::RemoveRewards()
{
	if (ULyraAbilitySystemComponent* ASC = RewardASC.Get())
	{
		for (const FActiveGameplayEffectHandle Handle : RewardHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
	RewardHandles.Reset();
	RewardASC.Reset();
	RewardAvatar.Reset();
	AppliedLevel = 1;
}

void URunProgressionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetOwner()->HasAuthority())
	{
		RemoveRewards();
	}
	Super::EndPlay(Reason);
}
