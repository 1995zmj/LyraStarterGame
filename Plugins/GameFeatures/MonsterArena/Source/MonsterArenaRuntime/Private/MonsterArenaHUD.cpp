#include "MonsterArenaComponent.h"

#include "MonsterArenaRewardScreen.h"
#include "RunProgressionComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Character/LyraHealthComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Input/LyraInputConfig.h"
#include "Player/LyraPlayerState.h"
#include "PrimaryGameLayout.h"
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

TSharedRef<SWidget> UMonsterArenaHUD::RebuildWidget()
{
	if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this); }
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Box);
	CanvasSlot->SetPosition(FVector2D(32.0f, 120.0f));
	CanvasSlot->SetSize(FVector2D(480.0f, 150.0f));
	Status = WidgetTree->ConstructWidget<UTextBlock>();
	Box->AddChildToVerticalBox(Status);
	ExperienceBar = WidgetTree->ConstructWidget<UProgressBar>();
	ExperienceBar->SetFillColorAndOpacity(FLinearColor(0.1f, 0.8f, 0.6f));
	Box->AddChildToVerticalBox(ExperienceBar);
	Reward = WidgetTree->ConstructWidget<UTextBlock>();
	Reward->SetAutoWrapText(true);
	Reward->SetColorAndOpacity(FLinearColor(0.35f, 1.0f, 0.75f));
	Box->AddChildToVerticalBox(Reward);
	return Super::RebuildWidget();
}

void UMonsterArenaHUD::NativeConstruct()
{
	Super::NativeConstruct();
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->IsLocalController())
	{
		InputTracker = MakeShared<FMonsterArenaRewardInputTracker>(PC);
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().RegisterInputPreProcessor(InputTracker, 0);
			FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &ThisClass::HandleApplicationActivationChanged);
		}
		OpenRewardAction = NewObject<UMonsterArenaRewardInputAction>(this);
		RewardMapping = NewObject<UInputMappingContext>(this);
		RewardMapping->MapKey(OpenRewardAction, EKeys::U);
		RewardMapping->MapKey(OpenRewardAction, EKeys::Gamepad_DPad_Up);
		RewardInput = NewObject<UEnhancedInputComponent>(PC);
		RewardInput->RegisterComponent();
		RewardInput->BindAction(OpenRewardAction, ETriggerEvent::Started, this, &ThisClass::ToggleRewardMenu);
		PC->PushInputComponent(RewardInput);
		EnsureInputMapping();
	}
}

void UMonsterArenaHUD::EnsureInputMapping()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (Subsystem && RewardMapping && !Subsystem->HasMappingContext(RewardMapping))
	{
		FModifyContextOptions Options;
		Options.bNotifyUserSettings = true;
		Options.bIgnoreAllPressedKeysUntilRelease = true;
		Subsystem->AddMappingContext(RewardMapping, 100, Options);
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
	EnsureInputMapping();
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

void UMonsterArenaHUD::ToggleRewardMenu()
{
	if (IsRewardMenuOpen() || bRewardOpenRequested) { CloseRewardMenu(); }
	else { OpenRewardMenu(); }
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
	UClass* ScreenClass = RewardScreenClass ? RewardScreenClass.Get() : UMonsterArenaRewardScreen::StaticClass();
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
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;
			Subsystem->RemoveMappingContext(RewardMapping, Options);
		}
	}
	if (RewardInput)
	{
		if (APlayerController* PC = GetOwningPlayer()) { PC->PopInputComponent(RewardInput); }
		RewardInput->DestroyComponent();
		RewardInput = nullptr;
	}
	RewardMapping = nullptr;
	OpenRewardAction = nullptr;
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
