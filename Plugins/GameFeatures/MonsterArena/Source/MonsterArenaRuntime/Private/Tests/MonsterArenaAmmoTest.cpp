#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Player/LyraPlayerState.h"
#include "GameFramework/PlayerController.h"

#if WITH_DEV_AUTOMATION_TESTS
// Drive real automatic-fire input over world ticks, including misses and empty magazines.
void TickMonsterArenaAmmoTest(ALyraPlayerState* Player, float DeltaTime)
{
	static int32 Stage = 0;
	static float Elapsed = 0.0f;
	static int32 InitialAmmo = 0;
	static int32 ReleasedAmmo = 0;
	static bool bPassed = true;
	Elapsed += DeltaTime;
	const FGameplayTag Fire = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.FireAuto"));
	const FGameplayTag Reload = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Weapon.Reload"));
	const FGameplayTag Ammo = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineAmmo"));
	const FGameplayTag Spare = FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo"));
	ULyraQuickBarComponent* QuickBar = Player->GetPlayerController()->FindComponentByClass<ULyraQuickBarComponent>();
	ULyraInventoryItemInstance* Item = QuickBar ? QuickBar->GetActiveSlotItem() : nullptr;
	ULyraAbilitySystemComponent* ASC = Player->GetLyraAbilitySystemComponent();
	if (!Item || !ASC)
	{
		return;
	}
	auto Verify = [](bool bCondition, const TCHAR* Description) {
		UE_LOG(LogTemp, Display, TEXT("MonsterArenaAmmoTest: %s: %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), Description);
		bPassed &= bCondition;
	};
	if (Stage == 0 && Elapsed >= 2.0f)
	{
		ULyraEquipmentManagerComponent* Equipment = Player->GetPawn()->FindComponentByClass<ULyraEquipmentManagerComponent>();
		UClass* EquipmentClass = FindObject<UClass>(nullptr, TEXT("/Script/LyraGame.LyraEquipmentInstance"));
		const TArray<ULyraEquipmentInstance*> Instances = Equipment->GetEquipmentInstancesOfType(EquipmentClass);
		Verify(Instances.Num() == 1, TEXT("Exactly one weapon equipped"));
		Verify(Instances.Num() == 1 && Instances[0]->GetInstigator() == Item, TEXT("Equipped weapon uses quickbar ammo item"));
		Verify(Item->GetItemDef()->GetPathName().StartsWith(TEXT("/MonsterArena/Weapons/")), TEXT("Quickbar uses MonsterArena-owned item definition"));
		InitialAmmo = Item->GetStatTagStackCount(Ammo);
		Verify(InitialAmmo == 30, TEXT("Rifle starts with 30 rounds"));
		ASC->AbilityInputTagPressed(Fire);
		Stage = 1;
		Elapsed = 0.0f;
	}
	else if (Stage == 1 && Elapsed >= 1.0f)
	{
		ASC->AbilityInputTagReleased(Fire);
		Verify(Item->GetStatTagStackCount(Ammo) < InitialAmmo, TEXT("Holding fire consumes rounds even when missing"));
		Stage = 2;
		Elapsed = 0.0f;
	}
	else if (Stage == 2 && Elapsed >= 1.0f)
	{
		ReleasedAmmo = Item->GetStatTagStackCount(Ammo);
		Stage = 3;
		Elapsed = 0.0f;
	}
	else if (Stage == 3 && Elapsed >= 1.0f)
	{
		Verify(Item->GetStatTagStackCount(Ammo) == ReleasedAmmo, TEXT("Releasing fire stops consuming ammo"));
		Item->RemoveStatTagStack(Ammo, Item->GetStatTagStackCount(Ammo));
		Item->AddStatTagStack(Ammo, 1);
		Item->RemoveStatTagStack(Spare, Item->GetStatTagStackCount(Spare));
		ASC->AbilityInputTagPressed(Fire);
		Stage = 4;
		Elapsed = 0.0f;
	}
	else if (Stage == 4 && Elapsed >= 2.0f)
	{
		Verify(Item->GetStatTagStackCount(Ammo) == 0, TEXT("Last round is consumed and empty magazine stays empty"));
		int32 ActiveFireAbilities = 0;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetName().Contains(TEXT("GA_Weapon_Fire")) && Spec.IsActive())
			{
				++ActiveFireAbilities;
			}
		}
		Verify(ActiveFireAbilities == 0, TEXT("Empty magazine cannot keep firing"));
		ASC->AbilityInputTagReleased(Fire);
		Item->AddStatTagStack(Spare, 30);
		ASC->AbilityInputTagPressed(Reload);
		Stage = 5;
		Elapsed = 0.0f;
	}
	else if (Stage == 5 && Elapsed >= 0.5f)
	{
		ASC->AbilityInputTagReleased(Reload);
		Stage = 6;
		Elapsed = 0.0f;
	}
	else if (Stage == 6 && Elapsed >= 4.0f)
	{
		Verify(Item->GetStatTagStackCount(Ammo) == 30 && Item->GetStatTagStackCount(Spare) == 0, TEXT("Reload transfers reserve ammo into magazine"));
		UE_LOG(LogTemp, Display, TEXT("MONSTER_ARENA_AMMO_%s"), bPassed ? TEXT("SUCCEEDED") : TEXT("FAILED"));
		FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
		Stage = 7;
	}
	ASC->ProcessAbilityInput(DeltaTime, false);
}
#endif
