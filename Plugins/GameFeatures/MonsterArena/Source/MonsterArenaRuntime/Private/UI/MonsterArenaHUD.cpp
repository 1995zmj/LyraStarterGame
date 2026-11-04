#include "UI/MonsterArenaHUD.h"

#include "UI/MonsterArenaCoreHealthWidget.h"
#include "RunProgressionComponent.h"
#include "ArenaWaveDirectorComponent.h"
#include "ArenaDefenseCore.h"
#include "GameFramework/GameStateBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

#define LOCTEXT_NAMESPACE "MonsterArenaHUD"

namespace
{
	/** Builds the current hint text from the live mapping so rebinding updates the HUD. */
	FText MakeKeyHint(const ULocalPlayer* LocalPlayer, const UInputAction* Action, const FText& Fallback)
	{
		if (!LocalPlayer || !Action) { return Fallback; }
		const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (!Subsystem) { return Fallback; }
		TArray<FString> Labels;
		for (const FKey& Key : Subsystem->QueryKeysMappedToAction(Action))
		{
			FText Label = Key.GetDisplayName();
			if (Key == EKeys::Gamepad_DPad_Up) { Label = LOCTEXT("DPadUp", "手柄↑"); }
			else if (Key == EKeys::Gamepad_DPad_Down) { Label = LOCTEXT("DPadDown", "手柄↓"); }
			else if (Key == EKeys::Gamepad_DPad_Left) { Label = LOCTEXT("DPadLeft", "手柄←"); }
			else if (Key == EKeys::Gamepad_DPad_Right) { Label = LOCTEXT("DPadRight", "手柄→"); }
			Labels.AddUnique(Label.ToString());
		}
		return Labels.IsEmpty() ? Fallback : FText::FromString(FString::Join(Labels, TEXT(" / ")));
	}
}

void UMonsterArenaHUD::RefreshCoreHealth()
{
	if (!CoreHealthDisplay) { return; }
	const UArenaWaveDirectorComponent* Director = nullptr;
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			Director = GameState->FindComponentByClass<UArenaWaveDirectorComponent>();
		}
	}
	const AArenaDefenseCore* Core = Director ? Director->Core.Get() : nullptr;
	CoreHealthDisplay->SetCoreHealth(Core ? Core->Health : 0.0f, Core ? Core->MaxHealth : 0.0f, Core != nullptr);
}

void UMonsterArenaHUD::RefreshProgression()
{
	RefreshCoreHealth();
	if (!BoundProgression || !Status || !Reward) { return; }
	const FRunProgressionState& State = BoundProgression->State;
	const int32 Required = BoundProgression->GetExperienceRequired();
	Status->SetText(FText::Format(LOCTEXT("Status", "等级 {0}  |  经验 {1}/{2}  |  击杀 {3}"), FText::AsNumber(State.Level), FText::AsNumber(State.Experience), FText::AsNumber(Required), FText::AsNumber(State.Kills)));
	if (BoundProgression->GetDamagePerKill() > 0.0f)
	{
		Status->SetText(FText::Format(LOCTEXT("StatusAccumulation", "{0}\n火力积累：每次击杀 +{1}  |  已累积伤害 +{2}"), Status->GetText(), FText::AsNumber(BoundProgression->GetDamagePerKill()), FText::AsNumber(State.AccumulatedKillDamage)));
	}
	ExperienceBar->SetPercent(Required > 0 ? float(State.Experience) / Required : 1.0f);
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const FText RewardKeys = MakeKeyHint(LocalPlayer, OpenRewardAction, LOCTEXT("DefaultRewardKeys", "U / 手柄↑"));
	const FText WaveKeys = MakeKeyHint(LocalPlayer, WaveAction, LOCTEXT("DefaultWaveKeys", "N / 手柄→"));
	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		if (const auto* Director = GS->FindComponentByClass<UArenaWaveDirectorComponent>())
		{
			const FArenaWaveState& Wave = Director->State;
			FText Phase;
			switch (Wave.Phase)
			{
			case EArenaWavePhase::Preparation: Phase = FText::Format(LOCTEXT("WavePrepare", "准备：靠近核心，{0} 开始"), WaveKeys); break;
			case EArenaWavePhase::Warning: Phase = FText::Format(LOCTEXT("WaveWarning", "进攻预警 · 入口 {0}"), FText::FromString(Wave.EntranceNames)); break;
			case EArenaWavePhase::Spawning: Phase = LOCTEXT("WaveSpawning", "敌军进攻中"); break;
			case EArenaWavePhase::Clearing: Phase = LOCTEXT("WaveClearing", "清理剩余敌人"); break;
			case EArenaWavePhase::Intermission: Phase = FText::Format(LOCTEXT("WaveRest", "波间整备：靠近核心，{0} 下一波"), WaveKeys); break;
			case EArenaWavePhase::Victory: Phase = FText::Format(LOCTEXT("WaveWin", "防守胜利 · {0} 重置对局"), WaveKeys); break;
			case EArenaWavePhase::Defeat: Phase = FText::Format(LOCTEXT("WaveLose", "防守失败 · 复活后 {0} 重置对局"), WaveKeys); break;
			}
			if (Wave.bSpawnBlocked) { Phase = LOCTEXT("WaveBlocked", "出怪受阻：请离开入口；若持续受阻，请检查导航和出生空间"); }
			Status->SetText(FText::Format(LOCTEXT("WaveStatus", "{0}\n波次 {1}/{2}  |  存活 {3}  待生成 {4}\n{5}"),
				Status->GetText(),
				FText::AsNumber(Wave.WaveIndex), FText::AsNumber(Wave.WaveCount), FText::AsNumber(Wave.AliveCount), FText::AsNumber(Wave.PendingCount), Phase));
		}
	}
	if (State.Level > LastLevel) { NotificationRemaining = 5.0f; }
	LastLevel = State.Level;
	const float HintBrightness = NotificationRemaining > 0.0f ? 0.82f + 0.18f * FMath::Cos(NotificationRemaining * 8.0f) : 0.82f;
	Reward->SetRenderOpacity(HintBrightness);
	Reward->SetText(State.PendingChoices > 0 ? FText::Format(LOCTEXT("Available", "可选强化 ×{0}  ·  按 {1} 打开"), FText::AsNumber(State.PendingChoices), RewardKeys) : LOCTEXT("NoRewards", "击杀怪物获取经验，升级后可选择强化"));
}

#undef LOCTEXT_NAMESPACE
