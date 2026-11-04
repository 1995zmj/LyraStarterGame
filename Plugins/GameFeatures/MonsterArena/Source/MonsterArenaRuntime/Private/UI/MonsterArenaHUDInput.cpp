#include "UI/MonsterArenaHUD.h"

#include "UI/MonsterArenaRewardScreen.h"
#include "RunProgressionComponent.h"
#include "InputAction.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraHealthComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Input/LyraInputConfig.h"
#include "InputMappingContext.h"
#include "Player/LyraPlayerState.h"
#include "PrimaryGameLayout.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "MonsterArenaHUD"

/** Tracks physical releases before CommonUI can consume/flush the gameplay input. */
class FMonsterArenaRewardInputTracker : public IInputProcessor
{
public:
	explicit FMonsterArenaRewardInputTracker(APlayerController* Player) : Controller(Player) {}
	virtual void Tick(float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
	{
		if (IsLocalUser(Event.GetUserIndex())) { HeldKeys.Add(Event.GetKey()); }
		return false;
	}
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
	{
		if (IsLocalUser(Event.GetUserIndex())) { HeldKeys.Remove(Event.GetKey()); }
		return false;
	}
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		if (IsLocalUser(Event.GetUserIndex())) { HeldKeys.Add(Event.GetEffectingButton()); }
		return false;
	}
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		if (IsLocalUser(Event.GetUserIndex())) { HeldKeys.Remove(Event.GetEffectingButton()); }
		return false;
	}
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& Event) override
	{
		if (IsLocalUser(Event.GetUserIndex()) && (Event.GetKey() == EKeys::Gamepad_RightTriggerAxis || Event.GetKey() == EKeys::Gamepad_LeftTriggerAxis))
		{
			if (Event.GetAnalogValue() > 0.1f) { HeldKeys.Add(Event.GetKey()); }
			else { HeldKeys.Remove(Event.GetKey()); }
		}
		return false;
	}
	void SetFireKeys(const TSet<FKey>& Keys) { FireKeys = Keys; }
	void ClearHeldKeys() { HeldKeys.Reset(); }
	bool IsReleased() const
	{
		for (const FKey& Key : FireKeys) { if (HeldKeys.Contains(Key)) { return false; } }
		return true;
	}
private:
	bool IsLocalUser(uint32 UserIndex) const
	{
		const ULocalPlayer* LocalPlayer = Controller.IsValid() ? Controller->GetLocalPlayer() : nullptr;
		const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
		return SlateUser && int32(UserIndex) == SlateUser->GetUserIndex();
	}
	TWeakObjectPtr<APlayerController> Controller;
	TSet<FKey> HeldKeys;
	TSet<FKey> FireKeys;
};

void UMonsterArenaHUD::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->IsLocalController())
	{
		InputTracker = MakeShared<FMonsterArenaRewardInputTracker>(PC);
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().RegisterInputPreProcessor(InputTracker, 0);
			FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &ThisClass::HandleApplicationActivationChanged);
		}
		OpenRewardAction = OpenRewardActionAsset.LoadSynchronous();
		WaveAction = WaveActionAsset.LoadSynchronous();
		if (!OpenRewardAction || !WaveAction)
		{
			UE_LOG(LogTemp, Error, TEXT("MonsterArena: input actions are missing; regenerate them with Content/Python/setup_monster_arena_input.py."));
		}
		// The experience action can run before the IMC asset is loaded; register it here
		// once the local player and its key profile definitely exist.
		if (UInputMappingContext* Mapping = RewardMappingAsset.LoadSynchronous())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
			{
				if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
				{
					Settings->RegisterInputMappingContext(Mapping);
				}
			}
		}
	}
}

bool UMonsterArenaHUD::IsRewardToggleKey(const FKey& Key) const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return Subsystem && OpenRewardAction && Subsystem->QueryKeysMappedToAction(OpenRewardAction).Contains(Key);
}

void UMonsterArenaHUD::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	const APlayerController* PC = GetOwningPlayer();
	if (PC && InputPawn.Get() != PC->GetPawn())
	{
		CloseRewardMenu();
		ReleaseRewardInputBlock();
		if (InputTracker) { InputTracker->ClearHeldKeys(); }
		InputPawn = PC->GetPawn();
	}
	if (InputTracker && FSlateApplication::IsInitialized())
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const TSharedPtr<SViewport> Viewport = LocalPlayer && LocalPlayer->ViewportClient ? LocalPlayer->ViewportClient->GetGameViewportWidget() : nullptr;
		const TSharedPtr<SWindow> Window = Viewport ? FSlateApplication::Get().FindWidgetWindow(Viewport.ToSharedRef()) : nullptr;
		if (Window && bViewportActive != Window->IsActive()) { HandleApplicationActivationChanged(Window->IsActive()); }
	}
	URunProgressionComponent* Progress = PC && PC->PlayerState ? PC->PlayerState->FindComponentByClass<URunProgressionComponent>() : nullptr;
	if (Progress != BoundProgression)
	{
		CloseRewardMenu();
		ReleaseRewardInputBlock();
		if (InputTracker) { InputTracker->ClearHeldKeys(); }
		if (BoundProgression) { BoundProgression->OnProgressionChanged.RemoveDynamic(this, &ThisClass::RefreshProgression); }
		BoundProgression = Progress;
		LastLevel = Progress ? Progress->State.Level : 1;
		if (BoundProgression) { BoundProgression->OnProgressionChanged.AddUniqueDynamic(this, &ThisClass::RefreshProgression); }
		RefreshProgression();
	}
	if (IsRewardMenuOpen() && (!BoundProgression || !BoundProgression->CanOpenRewardChoices())) { CloseRewardMenu(); }
	const ULyraHealthComponent* Health = PC ? ULyraHealthComponent::FindHealthComponent(PC->GetPawn()) : nullptr;
	if (Health && Health->IsDeadOrDying())
	{
		ReleaseRewardInputBlock();
		if (InputTracker) { InputTracker->ClearHeldKeys(); }
	}
	if (BlockedASC.IsValid())
	{
		BlockedASC->ClearAbilityInput();
		if (bWaitForInputRelease && (!InputTracker || InputTracker->IsReleased())) { ReleaseRewardInputBlock(); }
	}
	if (bRewardOpenRequested) { TryOpenRewardMenu(); }
	NotificationRemaining = FMath::Max(0.0f, NotificationRemaining - DeltaTime);
	// Keep the input hint up to date after key rebinding without reopening the HUD.
	RefreshProgression();
}

void UMonsterArenaHUD::OpenRewardMenu()
{
	if (IsRewardMenuOpen() || !BoundProgression || !BoundProgression->CanOpenRewardChoices()) { return; }
	bRewardOpenRequested = true;
	TryOpenRewardMenu();
}

void UMonsterArenaHUD::TryOpenRewardMenu()
{
	if (IsRewardMenuOpen() || !BoundProgression || !BoundProgression->CanOpenRewardChoices())
	{
		bRewardOpenRequested = false;
		return;
	}
	// CommonUI returns a UObject to its pool before the outgoing Slate widget's
	// delayed destruction. Reusing it in that interval lets the old NativeDestruct
	// tear down the new session. Wait for the previous Slate lifetime to finish.
	if (RewardScreen && RewardScreen->GetCachedWidget().IsValid()) { return; }
	UPrimaryGameLayout* Layout = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer());
	if (!Layout)
	{
		if (Reward) { Reward->SetText(LOCTEXT("LayoutLoading", "强化界面正在加载，请稍后再试。")); }
		return;
	}
	const FGameplayTag ModalLayer = FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Modal"));
	UClass* ScreenClass = RewardScreenClass.LoadSynchronous();
	if (!ScreenClass)
	{
		bRewardOpenRequested = false;
		return;
	}
	RewardScreen = Layout->PushWidgetToLayerStack<UMonsterArenaRewardScreen>(ModalLayer, ScreenClass, [this](UMonsterArenaRewardScreen& Screen)
	{
		Screen.InitializeForProgression(BoundProgression, this);
	});
	bRewardOpenRequested = false;
}

void UMonsterArenaHUD::CloseRewardMenu()
{
	bRewardOpenRequested = false;
	if (RewardScreen)
	{
		RewardScreen->CloseSelection();
		if (UPrimaryGameLayout* Layout = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer()))
		{
			Layout->FindAndRemoveWidgetFromLayer(RewardScreen);
		}
	}
}

bool UMonsterArenaHUD::IsRewardMenuOpen() const
{
	return RewardScreen && RewardScreen->IsActivated();
}

void UMonsterArenaHUD::BeginRewardInputBlock()
{
	bWaitForInputRelease = false;
	if (InputTracker)
	{
		TSet<FKey> FireKeys;
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
		const ULyraPawnExtensionComponent* Extension = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwningPlayerPawn());
		const ULyraPawnData* PawnData = Extension ? Extension->GetPawnData<ULyraPawnData>() : nullptr;
		if (Subsystem && PawnData && PawnData->InputConfig)
		{
			for (const FLyraInputAction& Binding : PawnData->InputConfig->AbilityInputActions)
			{
				if (Binding.InputAction && Binding.InputTag.ToString().StartsWith(TEXT("InputTag.Weapon.Fire")))
				{
					for (const FKey& Key : Subsystem->QueryKeysMappedToAction(Binding.InputAction)) { FireKeys.Add(Key); }
				}
			}
		}
		if (FireKeys.IsEmpty()) { FireKeys.Add(EKeys::LeftMouseButton); FireKeys.Add(EKeys::Gamepad_RightTrigger); }
		if (FireKeys.Contains(EKeys::Gamepad_RightTrigger)) { FireKeys.Add(EKeys::Gamepad_RightTriggerAxis); }
		if (FireKeys.Contains(EKeys::Gamepad_LeftTrigger)) { FireKeys.Add(EKeys::Gamepad_LeftTriggerAxis); }
		InputTracker->SetFireKeys(FireKeys);
	}
	if (!BlockedASC.IsValid())
	{
		ALyraPlayerState* PS = GetOwningPlayer() ? GetOwningPlayer()->GetPlayerState<ALyraPlayerState>() : nullptr;
		BlockedASC = PS ? PS->GetLyraAbilitySystemComponent() : nullptr;
		if (BlockedASC.IsValid())
		{
			BlockedASC->AddLooseGameplayTag(TAG_Gameplay_AbilityInputBlocked);
			BlockedASC->CancelInputActivatedAbilities(true);
			BlockedASC->ClearAbilityInput();
		}
	}
}

void UMonsterArenaHUD::EndRewardInputBlock()
{
	bWaitForInputRelease = true;
	if (BlockedASC.IsValid()) { BlockedASC->ClearAbilityInput(); }
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			FModifyContextOptions Options;
			Options.bIgnoreAllPressedKeysUntilRelease = true;
			Subsystem->RequestRebuildControlMappings(Options);
		}
	}
}

void UMonsterArenaHUD::ReleaseRewardInputBlock()
{
	if (BlockedASC.IsValid())
	{
		BlockedASC->ClearAbilityInput();
		BlockedASC->RemoveLooseGameplayTag(TAG_Gameplay_AbilityInputBlocked);
	}
	BlockedASC.Reset();
	bWaitForInputRelease = false;
}

void UMonsterArenaHUD::HandleApplicationActivationChanged(bool bActive)
{
	bViewportActive = bActive;
	if (!bActive && InputTracker) { InputTracker->ClearHeldKeys(); }
	if (!IsRewardMenuOpen())
	{
		EndRewardInputBlock();
		ReleaseRewardInputBlock();
	}
}

void UMonsterArenaHUD::NativeDestruct()
{
	CloseRewardMenu();
	ReleaseRewardInputBlock();
	if (BoundProgression) { BoundProgression->OnProgressionChanged.RemoveDynamic(this, &ThisClass::RefreshProgression); }
	BoundProgression = nullptr;
	RewardScreen = nullptr;
	if (FSlateApplication::IsInitialized())
	{
		if (InputTracker) { FSlateApplication::Get().UnregisterInputPreProcessor(InputTracker); }
		FSlateApplication::Get().OnApplicationActivationStateChanged().RemoveAll(this);
	}
	InputTracker.Reset();
	OpenRewardAction = nullptr;
	WaveAction = nullptr;
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
