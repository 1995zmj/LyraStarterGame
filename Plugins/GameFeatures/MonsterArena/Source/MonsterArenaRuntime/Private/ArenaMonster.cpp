#include "ArenaMonster.h"
#include "ArenaDefenseCore.h"
#include "ArenaWaveDirectorComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Net/UnrealNetwork.h"

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
	if (WaveDirector.IsValid()) { WaveDirector->NotifyMonsterDeath(this); }
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
	if (DefenseTarget && Target == DefenseTarget) { return IsValid(DefenseTarget) && DefenseTarget->Health > 0; }
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
		if (AArenaDefenseCore* Core = Cast<AArenaDefenseCore>(Target)) { Core->ApplyEnemyDamage(MeleeDamage); return; }
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

void AArenaMonster::UpdateNavigation(float DeltaTime)
{
	if (!IsAliveForAI() || !IsValidAITarget(AITarget.Get())) { return; }
	// Standalone legacy monsters keep their existing open-arena behavior.
	FVector Destination = AITarget->GetActorLocation();
	if (DefenseTarget)
	{
		PathRemaining -= DeltaTime;
		if (PathRemaining <= 0)
		{
			PathRemaining = 0.5f;
			if (FVector::DistSquared2D(LastNavigationPosition, GetActorLocation()) < FMath::Square(10.0f)) { StuckTime += 0.5f; }
			else { StuckTime = 0; }
			LastNavigationPosition = GetActorLocation();
			if (StuckTime >= 15) { Destroy(); return; } // Director requeues without awarding XP.
			PathPoints.Reset(); PathIndex = 1;
			if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Destination, this))
			{
				if (Path->IsValid() && !Path->IsPartial()) { PathPoints = Path->PathPoints; }
			}
		}
		while (PathPoints.IsValidIndex(PathIndex) && FVector::DistSquared2D(GetActorLocation(), PathPoints[PathIndex]) < FMath::Square(50.0f)) { ++PathIndex; }
		if (!PathPoints.IsValidIndex(PathIndex)) { return; }
		Destination = PathPoints[PathIndex];
	}
	const FVector Direction = (Destination - GetActorLocation()).GetSafeNormal2D();
	SetActorRotation(Direction.Rotation());
	AddMovementInput(Direction, 1.0f, true);
}

APawn* AArenaMonster::ResolveWaveTarget(APawn* Candidate)
{
	if (!DefenseTarget || !IsValidAITarget(DefenseTarget)) { return Candidate; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (bHunter)
	{
		if (AITarget.IsValid() && AITarget.Get() != DefenseTarget && IsValidAITarget(AITarget.Get()) && Now < TargetHoldUntil) { return AITarget.Get(); }
		TargetHoldUntil = Now + 2;
		return Candidate ? Candidate : DefenseTarget.Get();
	}
	if (AITarget.IsValid() && AITarget.Get() != DefenseTarget && Now >= TargetHoldUntil) { IgnorePlayersUntil = Now + 2; }
	if (Now < IgnorePlayersUntil || !Candidate || FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation()) > FMath::Square(250.0f)) { return DefenseTarget; }
	if (AITarget.Get() == DefenseTarget || !AITarget.IsValid()) { TargetHoldUntil = Now + 3; }
	return Candidate;
}

void AArenaMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaMonster, bHunter);
}
