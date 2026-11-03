#include "UI/MonsterArenaCoreHealthWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "MonsterArenaCoreHealthWidget"

void UMonsterArenaCoreHealthWidget::SetCoreHealth(float CurrentHealth, float MaxHealth, bool bAvailable)
{
	if (!HealthText || !HealthBar)
	{
		return;
	}
	if (!bAvailable)
	{
		HealthText->SetText(LOCTEXT("Waiting", "大本营血量：等待同步"));
		HealthBar->SetPercent(0.0f);
		return;
	}
	const int32 CurrentValue = FMath::CeilToInt(FMath::Max(0.0f, CurrentHealth));
	const int32 MaxValue = FMath::CeilToInt(FMath::Max(0.0f, MaxHealth));
	HealthText->SetText(FText::Format(LOCTEXT("Health", "大本营血量：{0} / {1}"), FText::AsNumber(CurrentValue), FText::AsNumber(MaxValue)));
	HealthBar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f);
}

#undef LOCTEXT_NAMESPACE
