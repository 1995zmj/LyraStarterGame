#include "ArenaMonster.h"

#include "ArenaMeleeDamageEffect.h"
#include "Components/StateTreeComponent.h"
#include "StateTree.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "RunProgressionComponent.h"
#include "UObject/ConstructorHelpers.h"

AArenaMonster::AArenaMonster(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	AbilitySystem = CreateDefaultSubobject<ULyraAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	HealthSet = CreateDefaultSubobject<ULyraHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<ULyraCombatSet>(TEXT("CombatSet"));
	HealthComponent = CreateDefaultSubobject<UArenaMonsterHealthComponent>(TEXT("Health"));
	UStaticMeshComponent* Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.5f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body->SetStaticMesh(BodyMesh.Object);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = 220.0f;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);
}

UAbilitySystemComponent* AArenaMonster::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void UArenaMonsterHealthComponent::HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	// The owning monster settles its death through HealthSet::OnOutOfHealth.
}

void AArenaMonster::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(ULyraHealthSet::GetMaxHealthAttribute(), InitialHealth);
	}
	HealthComponent->InitializeWithAbilitySystem(AbilitySystem);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);
	if (HasAuthority())
	{
		StateTreeComponent->SetStateTree(StateTreeAsset.LoadSynchronous());
		StateTreeComponent->StartLogic();
	}
}

void AArenaMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StateTreeComponent->StopLogic(TEXT("Monster ended play"));
	AITarget.Reset();
	Super::EndPlay(EndPlayReason);
}

void AArenaMonster::HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	if (!HasAuthority() || bDeathSettled)
	{
		return;
	}
	bDeathSettled = true;
	StateTreeComponent->StopLogic(TEXT("Monster died"));
	AITarget.Reset();
	AActor* Source = Spec ? Spec->GetContext().GetOriginalInstigator() : InstigatorActor;
	APlayerState* Player = Cast<APlayerState>(Source);
	if (const APawn* Pawn = Cast<APawn>(Source))
	{
		Player = Pawn->GetPlayerState();
	}
	if (const AController* SourceController = Cast<AController>(Source))
	{
		Player = SourceController->PlayerState;
	}
	FGameplayTagContainer AssetTags;
	if (Spec)
	{
		Spec->GetAllAssetTags(AssetTags);
	}
	if (Player && Player != GetPlayerState() && Spec && !AssetTags.HasTag(TAG_Gameplay_DamageSelfDestruct))
	{
		if (URunProgressionComponent* Progress = Player->FindComponentByClass<URunProgressionComponent>())
		{
			Progress->RecordMonsterKill(this, ExperienceReward);
		}
	}
	HealthComponent->StartDeath();
	HealthComponent->FinishDeath();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	SetLifeSpan(0.2f);
}

bool AArenaMonster::IsAliveForAI() const
{
	return HasAuthority() && !bDeathSettled && HealthComponent->GetHealth() > 0.0f && !HealthComponent->IsDeadOrDying();
}

bool AArenaMonster::IsValidAITarget(const APawn* Target) const
{
	const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Target);
	return IsValid(Target) && Target->IsPlayerControlled() && Health && Health->GetHealth() > 0.0f && !Health->IsDeadOrDying()
		&& FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(DetectionRadius);
}

bool AArenaMonster::IsTargetInMeleeRange() const
{
	const APawn* Target = AITarget.Get();
	return IsValidAITarget(Target) && FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(MeleeRange);
}

void AArenaMonster::TryMeleeAttack()
{
	if (!IsAliveForAI() || !IsTargetInMeleeRange() || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}
	NextAttackTime = GetWorld()->GetTimeSeconds() + FMath::Max(AttackInterval, 0.1f);
	APawn* Target = AITarget.Get();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterMelee), false, this);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Params) || Hit.GetActor() == Target)
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			FGameplayEffectSpecHandle DamageSpec = AbilitySystem->MakeOutgoingSpec(UArenaMeleeDamageEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
			if (DamageSpec.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(TAG_MonsterArena_MeleeDamage, MeleeDamage);
				AbilitySystem->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
			}
		}
	}
}
