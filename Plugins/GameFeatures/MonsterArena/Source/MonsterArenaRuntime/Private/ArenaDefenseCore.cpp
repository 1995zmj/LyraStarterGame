#include "ArenaDefenseCore.h"
#include "ArenaWaveDirectorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AArenaDefenseCore::AArenaDefenseCore()
{
	bReplicates = true;
	UStaticMeshComponent* Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreBody"));
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body->SetStaticMesh(Mesh.Object);
	Body->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.8f));
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Body->SetCanEverAffectNavigation(false);
}

void AArenaDefenseCore::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) { ResetCore(); }
}

void AArenaDefenseCore::ResetCore()
{
	if (HasAuthority()) { Health = FMath::Max(1.0f, MaxHealth); ForceNetUpdate(); }
}

void AArenaDefenseCore::ApplyEnemyDamage(float Amount)
{
	if (!HasAuthority() || Health <= 0 || !FMath::IsFinite(Amount) || Amount <= 0) { return; }
	Health = FMath::Max(0.0f, Health - Amount);
	ForceNetUpdate();
	if (Health <= 0)
	{
		if (AGameStateBase* GS = GetWorld()->GetGameState())
		{
			if (auto* Director = GS->FindComponentByClass<UArenaWaveDirectorComponent>()) { Director->FailRun(); }
		}
	}
}

void AArenaDefenseCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaDefenseCore, Health);
	DOREPLIFETIME(AArenaDefenseCore, MaxHealth);
}
