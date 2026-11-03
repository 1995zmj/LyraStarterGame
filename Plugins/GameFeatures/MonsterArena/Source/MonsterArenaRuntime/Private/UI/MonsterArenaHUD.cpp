#include "UI/MonsterArenaHUD.h"

#include "RunProgressionComponent.h"
#include "ArenaWaveDirectorComponent.h"
#include "ArenaDefenseCore.h"
#include "GameFramework/GameStateBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"

#define LOCTEXT_NAMESPACE "MonsterArenaHUD"

void UMonsterArenaHUD::RefreshProgression()
{
	if (!BoundProgression || !Status || !Reward) { return; }
	const FRunProgressionState& State = BoundProgression->State;
	const int32 Required = BoundProgression->GetExperienceRequired();
	Status->SetText(FText::Format(LOCTEXT("Status", "等级 {0}  |  经验 {1}/{2}  |  击杀 {3}"), FText::AsNumber(State.Level), FText::AsNumber(State.Experience), FText::AsNumber(Required), FText::AsNumber(State.Kills)));
	if (BoundProgression->GetDamagePerKill() > 0.0f)
	{
		Status->SetText(FText::Format(LOCTEXT("StatusAccumulation", "{0}\n火力积累：每次击杀 +{1}  |  已累积伤害 +{2}"), Status->GetText(), FText::AsNumber(BoundProgression->GetDamagePerKill()), FText::AsNumber(State.AccumulatedKillDamage)));
	}
	ExperienceBar->SetPercent(Required > 0 ? float(State.Experience) / Required : 1.0f);
	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		if (const auto* Director = GS->FindComponentByClass<UArenaWaveDirectorComponent>())
		{
			const FArenaWaveState& Wave = Director->State;
			FText Phase;
			switch (Wave.Phase)
			{
			case EArenaWavePhase::Preparation: Phase = LOCTEXT("WavePrepare", "准备：靠近核心，N / 手柄→ 开始"); break;
			case EArenaWavePhase::Warning: Phase = FText::Format(LOCTEXT("WaveWarning", "进攻预警 · 入口 {0}"), FText::FromString(Wave.EntranceNames)); break;
			case EArenaWavePhase::Spawning: Phase = LOCTEXT("WaveSpawning", "敌军进攻中"); break;
			case EArenaWavePhase::Clearing: Phase = LOCTEXT("WaveClearing", "清理剩余敌人"); break;
			case EArenaWavePhase::Intermission: Phase = LOCTEXT("WaveRest", "波间整备：靠近核心，N / 手柄→ 下一波"); break;
			case EArenaWavePhase::Victory: Phase = LOCTEXT("WaveWin", "防守胜利 · N / 手柄→ 重置对局"); break;
			case EArenaWavePhase::Defeat: Phase = LOCTEXT("WaveLose", "防守失败 · 复活后 N / 手柄→ 重置对局"); break;
			}
			if (Wave.bSpawnBlocked) { Phase = LOCTEXT("WaveBlocked", "出怪受阻：请离开入口；若持续受阻，请检查导航和出生空间"); }
			const FText CoreHealth = Director->Core
				? FText::Format(LOCTEXT("BaseHealth", "大本营血量：{0} / {1}"),
					FText::AsNumber(FMath::CeilToInt(FMath::Max(0.0f, Director->Core->Health))),
					FText::AsNumber(FMath::CeilToInt(FMath::Max(0.0f, Director->Core->MaxHealth))))
				: LOCTEXT("BaseUnavailable", "大本营血量：等待同步");
			Status->SetText(FText::Format(LOCTEXT("WaveStatusWithBaseHealth", "{0}\n{1}\n波次 {2}/{3}  |  存活 {4}  待生成 {5}\n{6}"),
				CoreHealth, Status->GetText(),
				FText::AsNumber(Wave.WaveIndex), FText::AsNumber(Wave.WaveCount), FText::AsNumber(Wave.AliveCount), FText::AsNumber(Wave.PendingCount), Phase));
		}
	}
	if (State.Level > LastLevel) { NotificationRemaining = 5.0f; }
	LastLevel = State.Level;
	const float HintBrightness = NotificationRemaining > 0.0f ? 0.82f + 0.18f * FMath::Cos(NotificationRemaining * 8.0f) : 0.82f;
	Reward->SetRenderOpacity(HintBrightness);
	FText Keys = LOCTEXT("DefaultRewardKeys", "U / 手柄↑");
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			TArray<FString> Labels;
			for (const FKey& Key : Subsystem->QueryKeysMappedToAction(OpenRewardAction))
			{
				FText Label = Key.GetDisplayName();
				if (Key == EKeys::Gamepad_DPad_Up) { Label = LOCTEXT("DPadUp", "手柄↑"); }
				else if (Key == EKeys::Gamepad_DPad_Down) { Label = LOCTEXT("DPadDown", "手柄↓"); }
				else if (Key == EKeys::Gamepad_DPad_Left) { Label = LOCTEXT("DPadLeft", "手柄←"); }
				else if (Key == EKeys::Gamepad_DPad_Right) { Label = LOCTEXT("DPadRight", "手柄→"); }
				Labels.AddUnique(Label.ToString());
			}
			if (!Labels.IsEmpty()) { Keys = FText::FromString(FString::Join(Labels, TEXT(" / "))); }
		}
	}
	Reward->SetText(State.PendingChoices > 0 ? FText::Format(LOCTEXT("Available", "可选强化 ×{0}  ·  按 {1} 打开"), FText::AsNumber(State.PendingChoices), Keys) : LOCTEXT("NoRewards", "击杀怪物获取经验，升级后可选择强化"));
}

#undef LOCTEXT_NAMESPACE
