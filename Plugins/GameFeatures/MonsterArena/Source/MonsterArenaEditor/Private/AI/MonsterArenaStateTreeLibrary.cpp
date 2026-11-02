#include "AI/MonsterArenaStateTreeLibrary.h"

#include "AI/MonsterArenaStateTreeNodes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StateTreeComponentSchema.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "StateTreeCompilerManager.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"

namespace
{
	void AddTargetCondition(UStateTreeState& State, bool bMelee, bool bInvert)
	{
		FMonsterArenaTargetCondition& Condition = State.AddEnterCondition<FMonsterArenaTargetCondition>().GetNode();
		Condition.bRequireMeleeRange = bMelee;
		Condition.bInvert = bInvert;
	}

	void AddReselectTransition(UStateTreeState& State, UStateTreeState& Root, bool bMelee, bool bInvert)
	{
		FStateTreeTransition& Transition = State.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Root);
		FMonsterArenaTargetCondition& Condition = Transition.AddConditionWithOuter<FMonsterArenaTargetCondition>(&State).GetNode();
		Condition.bRequireMeleeRange = bMelee;
		Condition.bInvert = bInvert;
	}
}

UStateTree* UMonsterArenaStateTreeLibrary::CreateMonsterStateTree()
{
	if (FPackageName::DoesPackageExist(TEXT("/MonsterArena/AI/ST_ArenaMonster")))
	{
		return LoadObject<UStateTree>(nullptr, TEXT("/MonsterArena/AI/ST_ArenaMonster.ST_ArenaMonster"));
	}
	UPackage* Package = CreatePackage(TEXT("/MonsterArena/AI/ST_ArenaMonster"));
	UStateTree* Tree = NewObject<UStateTree>(Package, TEXT("ST_ArenaMonster"), RF_Public | RF_Standalone | RF_Transactional);
	UStateTreeEditorData* Data = NewObject<UStateTreeEditorData>(Tree, NAME_None, RF_Transactional);
	Tree->EditorData = Data;
	Data->Schema = NewObject<UStateTreeComponentSchema>(Data);
	Data->AddEvaluator<FMonsterArenaFindTargetEvaluator>();
	UStateTreeState& Root = Data->AddRootState();
	UStateTreeState& Melee = Root.AddChildState(TEXT("Melee"));
	AddTargetCondition(Melee, true, false);
	Melee.AddTask<FMonsterArenaMeleeTask>();
	AddReselectTransition(Melee, Root, true, true);
	UStateTreeState& Chase = Root.AddChildState(TEXT("Chase"));
	AddTargetCondition(Chase, false, false);
	Chase.AddTask<FMonsterArenaChaseTask>();
	AddReselectTransition(Chase, Root, false, true);
	AddReselectTransition(Chase, Root, true, false);
	UStateTreeState& Idle = Root.AddChildState(TEXT("Idle"));
	Idle.AddTask<FMonsterArenaIdleTask>();
	AddReselectTransition(Idle, Root, false, false);
	Tree->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Tree);
	return Tree;
}

bool UMonsterArenaStateTreeLibrary::ValidateMonsterStateTree(UStateTree* Tree)
{
	const UStateTreeEditorData* Data = Tree ? Cast<UStateTreeEditorData>(Tree->EditorData) : nullptr;
	if (!Data || !Data->Schema || !Data->Schema->IsA<UStateTreeComponentSchema>())
	{
		return false;
	}
	FStateTreeCompilerLog Log;
	const bool bCompiled = UE::StateTree::Compiler::FCompilerManager::CompileSynchronously(Tree, Log);
	Log.DumpToLog(LogTemp);
	return bCompiled && Tree->LastCompiledEditorDataHash != 0 && !Tree->IsEditorDataDirty() && Tree->IsReadyToRun();
}
