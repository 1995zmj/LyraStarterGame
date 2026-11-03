#include "ArenaWaveContentLibrary.h"
#include "ArenaDefenseCore.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"

bool UArenaWaveContentLibrary::SetupDefenseMap(UWorld* World)
{
	if (!World || World->IsGameWorld()) { return false; }
	AArenaDefenseCore* Core = nullptr;
	for (TActorIterator<AArenaDefenseCore> It(World); It; ++It) { Core = *It; break; }
	if (!Core)
	{
		Core = World->SpawnActor<AArenaDefenseCore>(FVector(0, 600, 90), FRotator::ZeroRotator);
		if (!Core) { return false; }
		Core->SetActorLabel(TEXT("ArenaDefenseCore"));
		Core->MarkPackageDirty();
	}
	ANavMeshBoundsVolume* Bounds = nullptr;
	for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It) { Bounds = *It; break; }
	if (!Bounds)
	{
		Bounds = World->SpawnActor<ANavMeshBoundsVolume>();
		UCubeBuilder* Builder = NewObject<UCubeBuilder>();
		Builder->X = 3900; Builder->Y = 3900; Builder->Z = 1000;
		UActorFactory::CreateBrushForVolumeActor(Bounds, Builder);
		Bounds->SetActorLabel(TEXT("ArenaNavigationBounds"));
		Bounds->MarkPackageDirty();
	}
	if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		// Commandlets do not tick the editor callback that releases the async map-load lock.
		FlushAsyncLoading();
		World->BlockTillLevelStreamingCompleted();
		Nav->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock, UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
		Nav->OnNavigationBoundsUpdated(Bounds);
		Nav->Build();
		FNavLocation Projected;
		return Nav->ProjectPointToNavigation(Core->GetActorLocation(), Projected, FVector(100, 100, 250));
	}
	return false;
}
