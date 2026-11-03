#include "Tests/MonsterArenaWaveTestLibrary.h"
#include "RunProgressionComponent.h"
#include "ArenaWaveDirectorComponent.h"
#include "ArenaDefenseCore.h"
#include "Editor.h"

void UMonsterArenaWaveTestLibrary::RequestWave(URunProgressionComponent* Player, int32 RunId, int32 WaveIndex)
{
	TGuardValue<bool> RestoreGameplayCallspace(GAllowActorScriptExecutionInEditor, false);
	if (Player) { Player->RequestWaveAction(RunId, WaveIndex); }
}

void UMonsterArenaWaveTestLibrary::DamageCore(UArenaWaveDirectorComponent* Director, float Amount)
{
	if (Director && Director->Core) { Director->Core->ApplyEnemyDamage(Amount); }
}

void UMonsterArenaWaveTestLibrary::UseShortWaves(UArenaWaveDirectorComponent* Director)
{
	if (!Director || !Director->GetOwner()->HasAuthority()) { return; }
	Director->Definition = NewObject<UArenaWaveDefinition>(Director);
	for (FArenaWaveConfig& Wave : Director->Definition->Waves)
	{
		Wave.AssaultCount = 3; Wave.HunterCount = 1; Wave.AliveLimit = 2;
		Wave.BatchSize = 2; Wave.BatchInterval = 0.5f;
	}
	Director->Definition->WarningDuration = 0.3f;
	Director->ResetRun();
}
