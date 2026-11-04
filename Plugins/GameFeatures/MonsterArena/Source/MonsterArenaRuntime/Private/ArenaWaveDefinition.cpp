#include "ArenaWaveDefinition.h"

bool FArenaWaveMonsterEntry::IsValid() const
{
	return !MonsterId.IsNone() && Count >= 1 && Count <= 10000;
}

int32 FArenaWaveConfig::GetMonsterCount() const
{
	int64 Total = 0;
	for (const FArenaWaveMonsterEntry& Entry : Monsters) { Total += Entry.Count; }
	return static_cast<int32>(FMath::Min<int64>(Total, MAX_int32));
}

bool FArenaWaveConfig::IsValid() const
{
	if (Monsters.IsEmpty() || AliveLimit <= 0 || AliveLimit > 200 || BatchSize <= 0 || BatchSize > AliveLimit
		|| EntranceCount <= 0 || !FMath::IsFinite(BatchInterval) || BatchInterval < 0.3f)
	{
		return false;
	}
	TSet<FName> Seen;
	int64 Total = 0;
	for (const FArenaWaveMonsterEntry& Entry : Monsters)
	{
		if (!Entry.IsValid() || Seen.Contains(Entry.MonsterId)) { return false; }
		Seen.Add(Entry.MonsterId);
		Total += Entry.Count;
	}
	return Total > 0 && Total <= 10000;
}

UArenaWaveDefinition::UArenaWaveDefinition()
{
	FArenaMonsterConfig& Grunt = Monsters.AddDefaulted_GetRef();
	Grunt.MonsterId = TEXT("Grunt");
	Grunt.InitialHealth = 60.0f;
	Grunt.MeleeDamage = 10.0f;
	Grunt.MoveSpeed = 220.0f;
	Grunt.ExperienceReward = 20;
	Grunt.DetectionRadius = 4000.0f;
	Grunt.MeleeRange = 140.0f;
	Grunt.AttackInterval = 1.0f;
	Grunt.BodyScale = 1.0f;

	FArenaMonsterConfig& Runner = Monsters.AddDefaulted_GetRef();
	Runner.MonsterId = TEXT("Runner");
	Runner.InitialHealth = 40.0f;
	Runner.MeleeDamage = 6.0f;
	Runner.MoveSpeed = 320.0f;
	Runner.ExperienceReward = 15;
	Runner.DetectionRadius = 4500.0f;
	Runner.MeleeRange = 120.0f;
	Runner.AttackInterval = 0.8f;
	Runner.BodyScale = 0.75f;

	FArenaMonsterConfig& Brute = Monsters.AddDefaulted_GetRef();
	Brute.MonsterId = TEXT("Brute");
	Brute.InitialHealth = 160.0f;
	Brute.MeleeDamage = 24.0f;
	Brute.MoveSpeed = 150.0f;
	Brute.ExperienceReward = 45;
	Brute.DetectionRadius = 3500.0f;
	Brute.MeleeRange = 170.0f;
	Brute.AttackInterval = 1.5f;
	Brute.BodyScale = 1.45f;

	const int32 Limits[] = {6, 8, 12, 16, 20};
	const int32 Batches[] = {2, 3, 4, 5, 6};
	const float Intervals[] = {4, 4, 4, 3.5f, 3};
	const int32 Entrances[] = {1, 2, 2, 3, 4};
	const int32 Grunts[] = {12, 16, 16, 16, 18};
	const int32 Runners[] = {0, 4, 8, 12, 16};
	const int32 Brutes[] = {0, 0, 6, 12, 16};
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FArenaWaveConfig& Wave = Waves.AddDefaulted_GetRef();
		Wave.AliveLimit = Limits[Index];
		Wave.BatchSize = Batches[Index];
		Wave.BatchInterval = Intervals[Index];
		Wave.EntranceCount = Entrances[Index];
		if (Grunts[Index] > 0)
		{
			FArenaWaveMonsterEntry& Entry = Wave.Monsters.AddDefaulted_GetRef();
			Entry.MonsterId = TEXT("Grunt");
			Entry.Count = Grunts[Index];
		}
		if (Runners[Index] > 0)
		{
			FArenaWaveMonsterEntry& Entry = Wave.Monsters.AddDefaulted_GetRef();
			Entry.MonsterId = TEXT("Runner");
			Entry.Count = Runners[Index];
			Entry.bHunter = true;
		}
		if (Brutes[Index] > 0)
		{
			FArenaWaveMonsterEntry& Entry = Wave.Monsters.AddDefaulted_GetRef();
			Entry.MonsterId = TEXT("Brute");
			Entry.Count = Brutes[Index];
		}
	}
}

const FArenaMonsterConfig* UArenaWaveDefinition::FindMonster(FName MonsterId) const
{
	const FArenaMonsterConfig* Found = Monsters.FindByPredicate([MonsterId](const FArenaMonsterConfig& Config)
	{
		return Config.MonsterId == MonsterId;
	});
	return Found && Found->IsValid() ? Found : nullptr;
}
