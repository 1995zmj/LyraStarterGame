#include "UI/MonsterArenaRewardInputAction.h"

#include "PlayerMappableKeySettings.h"

#define LOCTEXT_NAMESPACE "MonsterArenaRewards"

UMonsterArenaRewardInputAction::UMonsterArenaRewardInputAction()
{
	ValueType = EInputActionValueType::Boolean;
	PlayerMappableKeySettings = CreateDefaultSubobject<UPlayerMappableKeySettings>(TEXT("RewardKeySettings"));
	PlayerMappableKeySettings->Name = TEXT("MonsterArena.OpenRewards");
	PlayerMappableKeySettings->DisplayName = LOCTEXT("OpenRewardsMapping", "打开强化选择");
	PlayerMappableKeySettings->DisplayCategory = LOCTEXT("InputCategory", "怪物竞技场");
}

#undef LOCTEXT_NAMESPACE
