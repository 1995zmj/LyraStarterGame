#pragma once

#include "InputAction.h"
#include "MonsterArenaRewardInputAction.generated.h"

/** Native action with a stable mapping name, exposed in Lyra's keyboard settings. */
UCLASS()
class MONSTERARENARUNTIME_API UMonsterArenaRewardInputAction : public UInputAction
{
	GENERATED_BODY()
public:
	UMonsterArenaRewardInputAction();
};
