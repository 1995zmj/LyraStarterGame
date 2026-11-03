#pragma once

#include "Blueprint/UserWidget.h"
#include "MonsterArenaCoreHealthWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** Displays a snapshot of the replicated defense-core health. Mounted by the HUD. */
UCLASS(Abstract, Blueprintable)
class MONSTERARENARUNTIME_API UMonsterArenaCoreHealthWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MonsterArena|HUD")
	void SetCoreHealth(float CurrentHealth, float MaxHealth, bool bAvailable);
protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|HUD", meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MonsterArena|HUD", meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
};
