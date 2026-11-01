#include "UI/MonsterArenaHUD.h"

#include "RunProgressionComponent.h"
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
