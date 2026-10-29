#include "RunProgressionComponent.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Character/LyraHealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "GameplayEffect.h"
#include "RunRewardEffects.h"
#include "Net/UnrealNetwork.h"
#include "Player/LyraPlayerState.h"

#define LOCTEXT_NAMESPACE "MonsterArenaProgression"

namespace
{
	bool IsValidCard(const FRunRewardOption& Card)
	{
		return !Card.CardId.IsNone()
			&& FMath::IsFinite(Card.MaxHealthBonus) && Card.MaxHealthBonus >= 0.0f
			&& FMath::IsFinite(Card.DamageBonus) && Card.DamageBonus >= 0.0f
			&& FMath::IsFinite(Card.HealAmount) && Card.HealAmount >= 0.0f
			&& FMath::IsFinite(Card.HealFraction) && Card.HealFraction >= 0.0f && Card.HealFraction <= 1.0f
			&& Card.SpareAmmo >= 0
			&& (Card.MaxHealthBonus > 0.0f || Card.DamageBonus > 0.0f || Card.HealAmount > 0.0f || Card.HealFraction > 0.0f || Card.SpareAmmo > 0);
	}
}

int32 URunProgressionDefinition::GetExperienceRequired(int32 Level) const
{
	return ExperiencePerLevel.IsValidIndex(Level - 1) ? FMath::Max(1, ExperiencePerLevel[Level - 1]) : 0;
}

URunProgressionComponent::URunProgressionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
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
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0 || GetExperienceRequired() == 0)
	{
		return;
	}
	int64 Remaining = int64(State.Experience) + Amount;
	while (GetExperienceRequired() > 0 && Remaining >= GetExperienceRequired())
	{
		Remaining -= GetExperienceRequired();
		++State.Level;
		++State.PendingChoices;
	}
	State.Experience = GetExperienceRequired() > 0 ? int32(Remaining) : 0;
	// Level-up never opens the screen, rolls cards or applies a reward.
	NotifyStateChanged();
}

void URunProgressionComponent::RecordMonsterKill(int32 ExperienceReward)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		++State.Kills;
		AddExperience(ExperienceReward);
		NotifyStateChanged();
	}
}

void URunProgressionComponent::ResetRun()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetRewardMenuOpen(false);
		RemoveRewards();
		const int32 NextRunId = State.RunId == MAX_int32 ? 1 : State.RunId + 1;
		State = FRunProgressionState();
		State.RunId = NextRunId;
		NotifyStateChanged();
	}
}

bool URunProgressionComponent::IsPlayerReady() const
{
	const ALyraPlayerState* Player = Cast<ALyraPlayerState>(GetOwner());
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Pawn);
	const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
	return Player && Player->GetPlayerController() && Pawn && Health && !Health->IsDeadOrDying() && Health->GetHealth() > 0.0f
		&& ASC && ASC->GetAvatarActor() == Pawn && ASC->GetSet<ULyraHealthSet>() && ASC->GetSet<ULyraCombatSet>();
}

bool URunProgressionComponent::CanOpenRewardChoices() const
{
	return State.PendingChoices > 0 && IsPlayerReady();
}

void URunProgressionComponent::RequestOpenRewardChoices()
{
	ServerSetRewardMenuOpen(State.RunId, true);
}

void URunProgressionComponent::RequestCloseRewardChoices()
{
	ServerSetRewardMenuOpen(State.RunId, false);
}

void URunProgressionComponent::RequestSelectReward(int32 RunId, int32 OfferId, FName CardId)
{
	ServerSelectReward(RunId, OfferId, CardId);
}

void URunProgressionComponent::ServerSetRewardMenuOpen_Implementation(int32 RunId, bool bOpen)
{
	if (RunId != State.RunId)
	{
		ClientRewardRequestFailed(LOCTEXT("RunChanged", "对局状态已更新，请重新打开强化界面。"));
		return;
	}
	if (!bOpen)
	{
		SetRewardMenuOpen(false);
		NotifyStateChanged();
		return;
	}
	if (!CanOpenRewardChoices() || !RefreshRewards())
	{
		ClientRewardRequestFailed(LOCTEXT("CannotOpen", "当前无法选择强化，请在角色存活且有待选奖励时重试。"));
		return;
	}
	if (State.Options.IsEmpty())
	{
		GenerateOffer();
	}
	SetRewardMenuOpen(true);
	NotifyStateChanged();
}

void URunProgressionComponent::SetRewardMenuOpen(bool bOpen)
{
	if (bOpen)
	{
		const ALyraPlayerState* Player = Cast<ALyraPlayerState>(GetOwner());
		ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
		if (!ASC || !IsPlayerReady())
		{
			return;
		}
		if (MenuBlockedASC != ASC)
		{
			SetRewardMenuOpen(false);
			MenuBlockedASC = ASC;
			ASC->AddLooseGameplayTag(TAG_Gameplay_AbilityInputBlocked);
		}
		ASC->CancelInputActivatedAbilities(true);
		ASC->ClearAbilityInput();
		MenuAvatar = Player->GetPawn();
	}
	else
	{
		if (ULyraAbilitySystemComponent* ASC = MenuBlockedASC.Get())
		{
			// Remove only the count owned by this component; other menus may still block input.
			ASC->RemoveLooseGameplayTag(TAG_Gameplay_AbilityInputBlocked);
			ASC->ClearAbilityInput();
		}
		MenuBlockedASC.Reset();
		MenuAvatar.Reset();
	}
	State.bRewardMenuOpen = bOpen;
}

ULyraInventoryItemInstance* URunProgressionComponent::FindRifleItem() const
{
	const ALyraPlayerState* Player = Cast<ALyraPlayerState>(GetOwner());
	const APlayerController* Controller = Player ? Player->GetPlayerController() : nullptr;
	const ULyraInventoryManagerComponent* Inventory = Controller ? Controller->FindComponentByClass<ULyraInventoryManagerComponent>() : nullptr;
	if (Inventory)
	{
		for (ULyraInventoryItemInstance* Item : Inventory->GetAllItems())
		{
			if (Item && Item->GetItemDef() && Item->GetItemDef()->GetPathName() == TEXT("/MonsterArena/Weapons/Rifle/ID_MonsterArenaRifle.ID_MonsterArenaRifle_C"))
			{
				return Item;
			}
		}
	}
	return nullptr;
}

bool URunProgressionComponent::IsCardApplicable(const FRunRewardOption& Card) const
{
	if (!IsValidCard(Card))
	{
		return false;
	}
	if (Card.SpareAmmo > 0)
	{
		const ULyraInventoryItemInstance* Item = FindRifleItem();
		if (!Item || int64(Item->GetStatTagStackCount(FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo")))) + Card.SpareAmmo > MAX_int32)
		{
			return false;
		}
	}
	const bool bOnlyHealing = Card.MaxHealthBonus == 0.0f && Card.DamageBonus == 0.0f && Card.SpareAmmo == 0;
	if (bOnlyHealing)
	{
		const ALyraPlayerState* Player = Cast<ALyraPlayerState>(GetOwner());
		const ULyraAbilitySystemComponent* ASC = Player ? Player->GetLyraAbilitySystemComponent() : nullptr;
		return ASC && ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()) < ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
	}
	return true;
}

void URunProgressionComponent::GenerateOffer()
{
	if (State.PendingChoices <= 0 || !State.Options.IsEmpty())
	{
		return;
	}
	const URunRewardPoolDefinition* Pool = Definition && Definition->RewardPool ? Definition->RewardPool.Get() : GetDefault<URunRewardPoolDefinition>();
	TArray<FRunRewardCardEntry> Eligible;
	TSet<FName> SeenIds;
	const int32 RewardLevel = State.ChosenRewards.Num() + 2;
	for (const FRunRewardCardEntry& Entry : Pool->Cards)
	{
		if (!FMath::IsFinite(Entry.Weight) || Entry.Weight <= 0.0f || Entry.MinimumLevel > RewardLevel || SeenIds.Contains(Entry.Option.CardId) || !IsCardApplicable(Entry.Option))
		{
			continue;
		}
		const int32 StackCount = State.ChosenRewards.FilterByPredicate([&Entry](const FRunRewardGrant& Grant) { return Grant.Card.CardId == Entry.Option.CardId; }).Num();
		if (Entry.MaxStacks > 0 && StackCount >= Entry.MaxStacks)
		{
			continue;
		}
		SeenIds.Add(Entry.Option.CardId);
		Eligible.Add(Entry);
	}
	// An empty/misconfigured/exhausted pool must never strand an earned choice.
	if (Eligible.IsEmpty())
	{
		FRunRewardCardEntry Fallback;
		Fallback.Option.CardId = TEXT("FallbackFirepower");
		Fallback.Option.Title = LOCTEXT("FallbackTitle", "基础火力强化");
		Fallback.Option.Description = LOCTEXT("FallbackDescription", "本局基础伤害 +3");
		Fallback.Option.DamageBonus = 3.0f;
		Eligible.Add(Fallback);
	}
	while (State.Options.Num() < 3 && !Eligible.IsEmpty())
	{
		double TotalWeight = 0.0;
		for (const FRunRewardCardEntry& Entry : Eligible)
		{
			TotalWeight += Entry.Weight;
		}
		double Roll = FMath::FRand() * TotalWeight;
		int32 ChosenIndex = Eligible.Num() - 1;
		for (int32 Index = 0; Index < Eligible.Num(); ++Index)
		{
			Roll -= Eligible[Index].Weight;
			if (Roll <= 0.0)
			{
				ChosenIndex = Index;
				break;
			}
		}
		State.Options.Add(Eligible[ChosenIndex].Option);
		Eligible.RemoveAt(ChosenIndex);
	}
	++State.OfferId;
}

void URunProgressionComponent::ServerSelectReward_Implementation(int32 RunId, int32 OfferId, FName CardId)
{
	if (bResolvingChoice || RunId != State.RunId || OfferId != State.OfferId || State.PendingChoices <= 0 || !State.bRewardMenuOpen)
	{
		ClientRewardRequestFailed(LOCTEXT("StaleChoice", "该次选择已处理或状态已更新，请选择当前显示的卡牌。"));
		return;
	}
	const FRunRewardOption* Found = State.Options.FindByPredicate([CardId](const FRunRewardOption& Option) { return Option.CardId == CardId; });
	if (!Found || !IsPlayerReady() || !IsCardApplicable(*Found))
	{
		ClientRewardRequestFailed(LOCTEXT("InvalidChoice", "这张卡牌当前无法使用，选卡次数已保留。"));
		return;
	}
	TGuardValue<bool> ResolveGuard(bResolvingChoice, true);
	const FRunRewardOption Card = *Found;
	ULyraInventoryItemInstance* RifleItem = Card.SpareAmmo > 0 ? FindRifleItem() : nullptr;
	if (Card.SpareAmmo > 0 && !RifleItem)
	{
		ClientRewardRequestFailed(LOCTEXT("RifleUnavailable", "步枪尚未就绪，选卡次数已保留。"));
		return;
	}
	if (!RefreshRewards())
	{
		ClientRewardRequestFailed(LOCTEXT("RewardNotReady", "角色属性尚未就绪，选卡次数已保留，请稍后重试。"));
		return;
	}
	TArray<FActiveGameplayEffectHandle> NewHandles;
	if (!ApplyPersistentReward(Card, NewHandles))
	{
		ClientRewardRequestFailed(LOCTEXT("RewardFailed", "强化暂时无法生效，选卡次数已保留。"));
		return;
	}
	// All fallible prerequisites were checked before applying permanent or instant rewards.
	ULyraAbilitySystemComponent* ASC = RewardASC.Get();
	if (Card.HealAmount > 0.0f || Card.HealFraction > 0.0f)
	{
		const float MaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
		const float Health = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
		ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), FMath::Min(Health + Card.HealAmount + MaxHealth * Card.HealFraction, MaxHealth));
	}
	if (Card.SpareAmmo > 0)
	{
		RifleItem->AddStatTagStack(FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo")), Card.SpareAmmo);
	}
	FRunRewardGrant& Grant = State.ChosenRewards.AddDefaulted_GetRef();
	Grant.OfferId = OfferId;
	Grant.RewardLevel = State.ChosenRewards.Num() + 1;
	Grant.Card = Card;
	RewardHandles.Append(NewHandles);
	AppliedRewardCount = State.ChosenRewards.Num();
	--State.PendingChoices;
	State.Options.Reset();
	if (State.PendingChoices > 0)
	{
		GenerateOffer();
	}
	else
	{
		SetRewardMenuOpen(false);
	}
	NotifyStateChanged();
}

void URunProgressionComponent::ClientRewardRequestFailed_Implementation(const FText& Reason)
{
	OnRewardRequestFailed.Broadcast(Reason);
}

void URunProgressionComponent::OnRep_State()
{
	OnProgressionChanged.Broadcast();
}

void URunProgressionComponent::NotifyStateChanged()
{
	GetOwner()->ForceNetUpdate();
	OnRep_State();
}

void URunProgressionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner()->HasAuthority())
	{
		const ALyraPlayerState* Player = Cast<ALyraPlayerState>(GetOwner());
		if (State.bRewardMenuOpen && (!IsPlayerReady() || !Player || MenuAvatar != Player->GetPawn()))
		{
			SetRewardMenuOpen(false);
			NotifyStateChanged();
		}
		RefreshRewards();
	}
}

bool URunProgressionComponent::ApplyPersistentReward(const FRunRewardOption& Card, TArray<FActiveGameplayEffectHandle>& OutHandles)
{
	ULyraAbilitySystemComponent* ASC = RewardASC.Get();
	if (!ASC)
	{
		return false;
	}
	auto Apply = [ASC, &OutHandles](UClass* EffectClass, float Magnitude)
	{
		if (Magnitude <= 0.0f)
		{
			return true;
		}
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ASC->MakeEffectContext());
		if (!Spec.IsValid())
		{
			return false;
		}
		Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("MonsterArena.Reward.Magnitude")), Magnitude);
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		if (!Handle.IsValid())
		{
			return false;
		}
		OutHandles.Add(Handle);
		return true;
	};
	if (!Apply(URunHealthRewardEffect::StaticClass(), Card.MaxHealthBonus) || !Apply(URunDamageRewardEffect::StaticClass(), Card.DamageBonus))
	{
		for (const FActiveGameplayEffectHandle Handle : OutHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
		OutHandles.Reset();
		return false;
	}
	return true;
}

bool URunProgressionComponent::RefreshRewards()
{
	if (!IsPlayerReady())
	{
		return false;
	}
	const ALyraPlayerState* Player = CastChecked<ALyraPlayerState>(GetOwner());
	ULyraAbilitySystemComponent* ASC = Player->GetLyraAbilitySystemComponent();
	const bool bRebuildRewards = RewardASC != ASC || RewardHandles.ContainsByPredicate([ASC](const FActiveGameplayEffectHandle Handle)
	{
		return ASC->GetActiveGameplayEffect(Handle) == nullptr;
	});
	const float HealthBeforeRebuild = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
	if (bRebuildRewards)
	{
		RemoveRewards();
		RewardASC = ASC;
	}
	// PlayerState's ASC survives respawn. Keep its valid effects so removing a
	// maximum-health bonus cannot clamp the new pawn's freshly initialized health.
	RewardAvatar = Player->GetPawn();
	bool bAllRewardsApplied = true;
	while (AppliedRewardCount < State.ChosenRewards.Num())
	{
		TArray<FActiveGameplayEffectHandle> NewHandles;
		if (!ApplyPersistentReward(State.ChosenRewards[AppliedRewardCount].Card, NewHandles))
		{
			bAllRewardsApplied = false;
			break;
		}
		RewardHandles.Append(NewHandles);
		++AppliedRewardCount;
	}
	if (bRebuildRewards)
	{
		// Rebuilding missing effects can temporarily lower MaxHealth. Restore only
		// health the pawn already had, never the cards' one-time healing amounts.
		const float PreservedHealth = FMath::Min(HealthBeforeRebuild, ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute()));
		if (!FMath::IsNearlyEqual(ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute()), PreservedHealth))
		{
			ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), PreservedHealth);
		}
	}
	// Healing and ammo are deliberately absent: a respawn only rebuilds permanent effects.
	return bAllRewardsApplied;
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
	AppliedRewardCount = 0;
}

void URunProgressionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetOwner()->HasAuthority())
	{
		SetRewardMenuOpen(false);
		RemoveRewards();
	}
	Super::EndPlay(Reason);
}

#undef LOCTEXT_NAMESPACE
