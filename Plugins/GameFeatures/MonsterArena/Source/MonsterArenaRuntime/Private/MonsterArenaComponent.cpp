#include "MonsterArenaComponent.h"

#include "ArenaMonster.h"
#include "RunProgressionComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentDefinition.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "GameFramework/PlayerController.h"
#include "Character/LyraHealthComponent.h"
#include "Player/LyraPlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if WITH_DEV_AUTOMATION_TESTS
void RunMonsterArenaSmokeTest(UWorld* World, ALyraPlayerState* Player);
void TickMonsterArenaAmmoTest(ALyraPlayerState* Player, float DeltaTime);
void TickMonsterArenaLifecycleTest(ALyraPlayerState* Player, float DeltaTime);
#endif

TSharedRef<SWidget> UMonsterArenaHUD::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Box);
	CanvasSlot->SetPosition(FVector2D(32.0f, 120.0f));
	CanvasSlot->SetSize(FVector2D(420.0f, 160.0f));
	Status = WidgetTree->ConstructWidget<UTextBlock>();
	Box->AddChildToVerticalBox(Status);
	ExperienceBar = WidgetTree->ConstructWidget<UProgressBar>();
	ExperienceBar->SetFillColorAndOpacity(FLinearColor(0.1f, 0.8f, 0.6f));
	Box->AddChildToVerticalBox(ExperienceBar);
	Reward = WidgetTree->ConstructWidget<UTextBlock>();
	Box->AddChildToVerticalBox(Reward);
	return Super::RebuildWidget();
}

void UMonsterArenaHUD::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	const APlayerController* PC = GetOwningPlayer();
	const URunProgressionComponent* Progress = PC && PC->PlayerState ? PC->PlayerState->FindComponentByClass<URunProgressionComponent>() : nullptr;
	if (!Progress || !Status)
	{
		return;
	}
	const int32 Required = Progress->GetExperienceRequired();
	Status->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d  |  XP %d / %d  |  Kills %d"), Progress->State.Level, Progress->State.Experience, Required, Progress->State.Kills)));
	ExperienceBar->SetPercent(Required > 0 ? float(Progress->State.Experience) / Required : 1.0f);
	if (Progress->State.Level > LastLevel)
	{
		Reward->SetText(FText::FromString(TEXT("LEVEL UP! Upgrade granted: even levels +20 health, odd levels +3 damage")));
		NotificationRemaining = 5.0f;
	}
	LastLevel = Progress->State.Level;
	NotificationRemaining -= DeltaTime;
	if (NotificationRemaining <= 0.0f)
	{
		Reward->SetText(FText::GetEmpty());
	}
}

UMonsterArenaComponent::UMonsterArenaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	MonsterClass = AArenaMonster::StaticClass();
}

void UMonsterArenaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	for (auto It = EquippedPawns.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			if (It.Value().bOwnsItem && It.Value().QuickBar.IsValid() && It.Value().Item.IsValid())
			{
				if (It.Value().QuickBar->GetSlots().IsValidIndex(It.Value().SlotIndex) && It.Value().QuickBar->GetSlots()[It.Value().SlotIndex] == It.Value().Item.Get())
				{
					It.Value().QuickBar->RemoveItemFromSlot(It.Value().SlotIndex);
				}
			}
			if (It.Value().bOwnsItem && It.Value().Inventory.IsValid() && It.Value().Item.IsValid())
			{
				It.Value().Inventory->RemoveItemInstance(It.Value().Item.Get());
			}
			It.RemoveCurrent();
		}
	}
	APawn* SpawnReference = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		if (PC->IsLocalController() && GetNetMode() != NM_DedicatedServer)
		{
			const bool bExists = HUDs.ContainsByPredicate([PC](const UMonsterArenaHUD* HUD) { return HUD && HUD->GetOwningPlayer() == PC; });
			if (!bExists)
			{
				UMonsterArenaHUD* HUD = CreateWidget<UMonsterArenaHUD>(PC, UMonsterArenaHUD::StaticClass());
				if (HUD)
				{
					HUD->AddToPlayerScreen();
					HUDs.Add(HUD);
				}
			}
		}
		if (!GetOwner()->HasAuthority())
		{
			continue;
		}
		APawn* Pawn = PC->GetPawn();
		const ALyraPlayerState* PS = PC->GetPlayerState<ALyraPlayerState>();
		if (!Pawn || !PS || PS->GetLyraAbilitySystemComponent()->GetAvatarActor() != Pawn)
		{
			continue;
		}
		const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Pawn);
		if (Health && Health->GetHealth() > 0.0f && !Health->IsDeadOrDying())
		{
			SpawnReference = Pawn;
		}
		if (EquippedPawns.Contains(Pawn))
		{
#if WITH_DEV_AUTOMATION_TESTS
			if (FParse::Param(FCommandLine::Get(), TEXT("MonsterArenaAmmoTest")))
			{
				TickMonsterArenaAmmoTest(const_cast<ALyraPlayerState*>(PS), DeltaTime);
			}
			if (FParse::Param(FCommandLine::Get(), TEXT("MonsterArenaLifecycleTest")))
			{
				TickMonsterArenaLifecycleTest(const_cast<ALyraPlayerState*>(PS), DeltaTime);
			}
#endif
			continue;
		}
		ULyraEquipmentManagerComponent* Equipment = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>();
		ULyraInventoryManagerComponent* Inventory = PC->FindComponentByClass<ULyraInventoryManagerComponent>();
		ULyraQuickBarComponent* QuickBar = PC->FindComponentByClass<ULyraQuickBarComponent>();
		if (!Equipment || !Inventory || !QuickBar)
		{
			continue;
		}
		UClass* RifleItem = LoadClass<ULyraInventoryItemDefinition>(nullptr, TEXT("/MonsterArena/Weapons/Rifle/ID_MonsterArenaRifle.ID_MonsterArenaRifle_C"));
		if (!RifleItem)
		{
			continue;
		}
		int32 SlotIndex = INDEX_NONE;
		ULyraInventoryItemInstance* Item = nullptr;
		const TArray<ULyraInventoryItemInstance*> Slots = QuickBar->GetSlots();
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Slots[Index] && Slots[Index]->GetItemDef() == RifleItem)
			{
				SlotIndex = Index;
				Item = Slots[Index];
				break;
			}
		}
		const bool bOwnsItem = Item == nullptr;
		if (!Item)
		{
			SlotIndex = QuickBar->GetNextFreeItemSlot();
			if (SlotIndex == INDEX_NONE)
			{
				continue;
			}
			Item = Inventory->AddItemDefinition(RifleItem);
			if (Item)
			{
				QuickBar->AddItemToSlot(SlotIndex, Item);
			}
		}
		if (Item)
		{
			QuickBar->SetActiveSlotIndex(SlotIndex);
			if (QuickBar->GetActiveSlotItem() == Item)
			{
				FArenaLoadout Loadout;
				Loadout.QuickBar = QuickBar;
				Loadout.SlotIndex = SlotIndex;
				Loadout.bOwnsItem = bOwnsItem;
				Loadout.Item = Item;
				Loadout.Inventory = Inventory;
				EquippedPawns.Add(Pawn, Loadout);
#if WITH_DEV_AUTOMATION_TESTS
				if (FParse::Param(FCommandLine::Get(), TEXT("MonsterArenaSmokeTest")))
				{
					RunMonsterArenaSmokeTest(GetWorld(), const_cast<ALyraPlayerState*>(PS));
				}
#endif
			}
			else if (bOwnsItem)
			{
				QuickBar->RemoveItemFromSlot(SlotIndex);
				Inventory->RemoveItemInstance(Item);
			}
		}
	}
#if WITH_DEV_AUTOMATION_TESTS
	if (FParse::Param(FCommandLine::Get(), TEXT("MonsterArenaAmmoTest")) || FParse::Param(FCommandLine::Get(), TEXT("MonsterArenaLifecycleTest")))
	{
		return;
	}
#endif
	if (!GetOwner()->HasAuthority() || !SpawnReference)
	{
		return;
	}
	Monsters.RemoveAll([](const TWeakObjectPtr<AArenaMonster>& Monster) { return !Monster.IsValid(); });
	SpawnCountdown -= DeltaTime;
	if (SpawnCountdown > 0.0f || Monsters.Num() >= MaxAliveMonsters || !MonsterClass)
	{
		return;
	}
	SpawnCountdown = FMath::Max(0.1f, SpawnInterval);
	TArray<AActor*> SpawnPoints;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MonsterSpawn")))
		{
			SpawnPoints.Add(*It);
		}
	}
	if (SpawnPoints.IsEmpty())
	{
		return;
	}
	const AActor* Point = SpawnPoints[FMath::RandHelper(SpawnPoints.Num())];
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	if (AArenaMonster* Monster = GetWorld()->SpawnActor<AArenaMonster>(MonsterClass, Point->GetActorTransform(), Params))
	{
		Monsters.Add(Monster);
	}
}

void UMonsterArenaComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	for (UMonsterArenaHUD* HUD : HUDs)
	{
		if (HUD)
		{
			HUD->RemoveFromParent();
		}
	}
	HUDs.Reset();
	if (GetOwner()->HasAuthority())
	{
		for (const TWeakObjectPtr<AArenaMonster>& Monster : Monsters)
		{
			if (Monster.IsValid())
			{
				Monster->Destroy();
			}
		}
		for (const auto& Pair : EquippedPawns)
		{
			if (Pair.Value.bOwnsItem && Pair.Value.QuickBar.IsValid() && Pair.Value.Item.IsValid())
			{
				if (Pair.Value.QuickBar->GetSlots().IsValidIndex(Pair.Value.SlotIndex) && Pair.Value.QuickBar->GetSlots()[Pair.Value.SlotIndex] == Pair.Value.Item.Get())
				{
					Pair.Value.QuickBar->RemoveItemFromSlot(Pair.Value.SlotIndex);
				}
			}
			if (Pair.Value.bOwnsItem && Pair.Value.Inventory.IsValid() && Pair.Value.Item.IsValid())
			{
				Pair.Value.Inventory->RemoveItemInstance(Pair.Value.Item.Get());
			}
		}
	}
	EquippedPawns.Reset();
	Super::EndPlay(Reason);
}
