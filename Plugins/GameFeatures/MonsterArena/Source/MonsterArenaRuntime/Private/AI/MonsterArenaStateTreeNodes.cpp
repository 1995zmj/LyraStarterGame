#include "AI/MonsterArenaStateTreeNodes.h"

#include "ArenaMonster.h"
#include "ArenaDefenseCore.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "StateTreeExecutionContext.h"

void FMonsterArenaFindTargetEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	Context.GetInstanceData(*this).SearchRemaining = 0.0f;
	Tick(Context, 0.0f);
}

void FMonsterArenaFindTargetEvaluator::TreeStop(FStateTreeExecutionContext& Context) const
{
	if (AArenaMonster* Monster = Cast<AArenaMonster>(Context.GetInstanceData(*this).Actor))
	{
		Monster->AITarget.Reset();
	}
}

void FMonsterArenaFindTargetEvaluator::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AArenaMonster* Monster = Cast<AArenaMonster>(Data.Actor);
	if (!Monster || !Monster->IsAliveForAI())
	{
		return;
	}
	Data.SearchRemaining -= DeltaTime;
	if (Data.SearchRemaining > 0.0f && (!Monster->AITarget.IsValid() || Monster->IsValidAITarget(Monster->AITarget.Get())))
	{
		return;
	}
	Data.SearchRemaining = FMath::Max(SearchInterval, 0.05f);
	APawn* Target = nullptr;
	float Nearest = FMath::Square(Monster->DetectionRadius);
	for (FConstPlayerControllerIterator It = Monster->GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Candidate = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (Monster->IsValidAITarget(Candidate))
		{
			const float Distance = FVector::DistSquared(Monster->GetActorLocation(), Candidate->GetActorLocation());
			if (Distance <= Nearest)
			{
				Nearest = Distance;
				Target = Candidate;
			}
		}
	}
	Monster->AITarget = Monster->ResolveWaveTarget(Target);
}

bool FMonsterArenaTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AArenaMonster* Monster = Cast<AArenaMonster>(Context.GetInstanceData(*this).Actor);
	const bool bMatches = Monster && Monster->IsAliveForAI() && (bRequireMeleeRange
		? Monster->IsTargetInMeleeRange() : Monster->IsValidAITarget(Monster->AITarget.Get()));
	return bMatches != bInvert;
}

EStateTreeRunStatus FMonsterArenaIdleTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AArenaMonster* Monster = Cast<AArenaMonster>(Context.GetInstanceData(*this).Actor))
	{
		Monster->ConsumeMovementInputVector();
		Monster->GetCharacterMovement()->StopMovementImmediately();
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMonsterArenaChaseTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	AArenaMonster* Monster = Cast<AArenaMonster>(Context.GetInstanceData(*this).Actor);
	if (Monster && Monster->IsAliveForAI() && Monster->IsValidAITarget(Monster->AITarget.Get()))
	{
		Monster->UpdateNavigation(DeltaTime);
	}
	return EStateTreeRunStatus::Running;
}

void FMonsterArenaChaseTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AArenaMonster* Monster = Cast<AArenaMonster>(Context.GetInstanceData(*this).Actor))
	{
		Monster->ConsumeMovementInputVector();
		Monster->GetCharacterMovement()->StopMovementImmediately();
	}
}

EStateTreeRunStatus FMonsterArenaMeleeTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return Tick(Context, 0.0f);
}

EStateTreeRunStatus FMonsterArenaMeleeTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	if (AArenaMonster* Monster = Cast<AArenaMonster>(Context.GetInstanceData(*this).Actor))
	{
		if (Monster->IsAliveForAI() && Monster->IsValidAITarget(Monster->AITarget.Get()))
		{
			Monster->SetActorRotation((Monster->AITarget->GetActorLocation() - Monster->GetActorLocation()).GetSafeNormal2D().Rotation());
		}
		Monster->TryMeleeAttack();
	}
	return EStateTreeRunStatus::Running;
}
