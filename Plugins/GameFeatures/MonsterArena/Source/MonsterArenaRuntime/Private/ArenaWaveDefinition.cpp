#include "ArenaWaveDefinition.h"

bool FArenaWaveConfig::IsValid() const
{
	return AssaultCount >= 0 && HunterCount >= 0 && int64(AssaultCount) + HunterCount > 0
		&& int64(AssaultCount) + HunterCount <= 10000 && AliveLimit > 0 && AliveLimit <= 200
		&& BatchSize > 0 && BatchSize <= AliveLimit && EntranceCount > 0
		&& FMath::IsFinite(BatchInterval) && BatchInterval >= 0.3f;
}

UArenaWaveDefinition::UArenaWaveDefinition()
{
	const int32 Assault[] = {12, 16, 22, 28, 34};
	const int32 Hunters[] = {0, 4, 8, 12, 16};
	const int32 Limits[] = {6, 8, 12, 16, 20};
	const int32 Batches[] = {2, 3, 4, 5, 6};
	const float Intervals[] = {4, 4, 4, 3.5f, 3};
	const int32 Entrances[] = {1, 2, 2, 3, 4};
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FArenaWaveConfig& Wave = Waves.AddDefaulted_GetRef();
		Wave.AssaultCount = Assault[Index]; Wave.HunterCount = Hunters[Index];
		Wave.AliveLimit = Limits[Index]; Wave.BatchSize = Batches[Index];
		Wave.BatchInterval = Intervals[Index]; Wave.EntranceCount = Entrances[Index];
	}
}
