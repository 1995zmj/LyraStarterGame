#pragma once

#include "StateTreeConditionBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeTaskBase.h"
#include "MonsterArenaStateTreeNodes.generated.h"

class AActor;

USTRUCT()
struct MONSTERARENARUNTIME_API FMonsterArenaAIContext
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> Actor = nullptr;
	UPROPERTY()
	float SearchRemaining = 0.0f;
};

/** Perception only; state selection belongs to the asset's conditions/transitions. */
USTRUCT(meta = (DisplayName = "Arena: Find nearest living player"))
struct MONSTERARENARUNTIME_API FMonsterArenaFindTargetEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FMonsterArenaAIContext;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void TreeStop(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.05"))
	float SearchInterval = 0.2f;
};

USTRUCT(meta = (DisplayName = "Arena: Target condition"))
struct MONSTERARENARUNTIME_API FMonsterArenaTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FMonsterArenaAIContext;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
	UPROPERTY(EditAnywhere, Category = Condition)
	bool bRequireMeleeRange = false;
	UPROPERTY(EditAnywhere, Category = Condition)
	bool bInvert = false;
};

USTRUCT(meta = (DisplayName = "Arena: Idle"))
struct MONSTERARENARUNTIME_API FMonsterArenaIdleTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	FMonsterArenaIdleTask()
	{
		bShouldCallTick = false;
	}
	using FInstanceDataType = FMonsterArenaAIContext;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta = (DisplayName = "Arena: Direct pursuit"))
struct MONSTERARENARUNTIME_API FMonsterArenaChaseTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FMonsterArenaAIContext;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta = (DisplayName = "Arena: Melee attack"))
struct MONSTERARENARUNTIME_API FMonsterArenaMeleeTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FMonsterArenaAIContext;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};
