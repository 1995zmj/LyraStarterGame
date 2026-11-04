#include "Input/MonsterArenaInputLibrary.h"

#include "AbilitySystem/LyraAbilitySet.h"
#include "Abilities/MonsterArenaOpenRewardsAbility.h"
#include "Abilities/MonsterArenaWaveActionAbility.h"
#include "AssetToolsModule.h"
#include "Engine/Blueprint.h"
#include "EnhancedActionKeyMapping.h"
#include "Factories/DataAssetFactory.h"
#include "GameFeatures/GameFeatureAction_AddAbilities.h"
#include "GameFeatures/GameFeatureAction_AddInputBinding.h"
#include "GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "GameModes/LyraExperienceActionSet.h"
#include "GameModes/LyraExperienceDefinition.h"
#include "IAssetTools.h"
#include "Input/LyraInputConfig.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputEditorModule.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "MonsterArenaGameplayTags.h"
#include "Player/LyraPlayerState.h"
#include "PlayerMappableKeySettings.h"
#include "UI/MonsterArenaRewardInputAction.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "MonsterArenaInput"

namespace
{
	const TCHAR* OpenRewardsActionPath = TEXT("/MonsterArena/Input/Actions/IA_MonsterArena_OpenRewards");
	const TCHAR* WaveActionPath = TEXT("/MonsterArena/Input/Actions/IA_MonsterArena_WaveAction");
	const TCHAR* MappingContextPath = TEXT("/MonsterArena/Input/Mappings/IMC_MonsterArena");
	const TCHAR* InputConfigPath = TEXT("/MonsterArena/Input/InputData_MonsterArena");
	const TCHAR* AbilitySetPath = TEXT("/MonsterArena/System/AbilitySet_MonsterArena");
	const TCHAR* ActionSetPath = TEXT("/MonsterArena/System/Experiences/LAS_MonsterArena_Components");
	const TCHAR* ExperienceBlueprintPath = TEXT("/MonsterArena/System/Experiences/B_MonsterArena");

	template <typename T>
	T* LoadAssetIfPresent(const TCHAR* PackagePath)
	{
		const FString FullPath = FString::Printf(TEXT("%s.%s"), PackagePath, *FPackageName::GetShortName(PackagePath));
		return LoadObject<T>(nullptr, *FullPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	UObject* CreateAsset(const TCHAR* PackagePath, UClass* AssetClass, UFactory* Factory)
	{
		const FString Path(PackagePath);
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		return AssetTools.CreateAsset(FPackageName::GetShortName(Path), FPackageName::GetLongPackagePath(Path), AssetClass, Factory);
	}

	UInputAction* CreateOrLoadAction(const TCHAR* PackagePath, TSubclassOf<UInputAction> ActionClass)
	{
		if (UInputAction* Existing = LoadAssetIfPresent<UInputAction>(PackagePath)) { return Existing; }
		UInputAction_Factory* Factory = NewObject<UInputAction_Factory>();
		Factory->InputActionClass = ActionClass;
		UInputAction* Action = Cast<UInputAction>(CreateAsset(PackagePath, ActionClass, Factory));
		if (Action)
		{
			Action->ValueType = EInputActionValueType::Boolean;
			Action->MarkPackageDirty();
		}
		return Action;
	}

	/** Adds the missing player-mappable metadata to a plain UInputAction asset. */
	void EnsurePlayerMappableSettings(UInputAction* Action, FName MappingName, const FText& DisplayName, const FText& DisplayCategory)
	{
		if (!Action) { return; }
		FObjectProperty* Property = FindFProperty<FObjectProperty>(UInputAction::StaticClass(), TEXT("PlayerMappableKeySettings"));
		if (!Property) { return; }
		UPlayerMappableKeySettings* Settings = Cast<UPlayerMappableKeySettings>(Property->GetObjectPropertyValue_InContainer(Action));
		if (!Settings)
		{
			Settings = NewObject<UPlayerMappableKeySettings>(Action, NAME_None, RF_Transactional);
			Property->SetObjectPropertyValue_InContainer(Action, Settings);
		}
		bool bChanged = false;
		if (Settings->Name != MappingName) { Settings->Name = MappingName; bChanged = true; }
		if (Settings->DisplayName.IsEmpty()) { Settings->DisplayName = DisplayName; bChanged = true; }
		if (Settings->DisplayCategory.IsEmpty()) { Settings->DisplayCategory = DisplayCategory; bChanged = true; }
		if (bChanged) { Action->MarkPackageDirty(); }
	}

	UInputMappingContext* CreateOrLoadMappingContext()
	{
		if (UInputMappingContext* Existing = LoadAssetIfPresent<UInputMappingContext>(MappingContextPath)) { return Existing; }
		UInputMappingContext_Factory* Factory = NewObject<UInputMappingContext_Factory>();
		Factory->InputMappingContextClass = UInputMappingContext::StaticClass();
		UInputMappingContext* Context = Cast<UInputMappingContext>(CreateAsset(MappingContextPath, UInputMappingContext::StaticClass(), Factory));
		if (Context) { Context->MarkPackageDirty(); }
		return Context;
	}

	template <typename T>
	T* CreateOrLoadDataAsset(const TCHAR* PackagePath, UClass* AssetClass)
	{
		if (T* Existing = LoadAssetIfPresent<T>(PackagePath)) { return Existing; }
		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = AssetClass;
		T* Asset = Cast<T>(CreateAsset(PackagePath, AssetClass, Factory));
		if (Asset) { Asset->MarkPackageDirty(); }
		return Asset;
	}

	void EnsureMapping(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		if (!Context || !Action) { return; }
		for (int32 Index = 0; Index < Context->GetMappings().Num(); ++Index)
		{
			FEnhancedActionKeyMapping& Mapping = Context->GetMapping(Index);
			if (Mapping.Key != Key) { continue; }
			if (Mapping.Action == Action) { return; }
			// The action asset was replaced in place; keep the existing mapping and only refresh its object pointer.
			if (Mapping.Action && Mapping.Action->GetPathName() == Action->GetPathName())
			{
				Mapping.Action = Action;
				Context->MarkPackageDirty();
				return;
			}
		}
		Context->MapKey(Action, Key);
		Context->MarkPackageDirty();
	}

	bool HasSingleMapping(const UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		int32 Count = 0;
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action == Action && Mapping.Key == Key) { ++Count; }
		}
		return Count == 1;
	}

	FArrayProperty* GetGrantedAbilitiesProperty()
	{
		FArrayProperty* Property = FindFProperty<FArrayProperty>(ULyraAbilitySet::StaticClass(), TEXT("GrantedGameplayAbilities"));
		if (!Property) { return nullptr; }
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property->Inner);
		return StructProperty && StructProperty->Struct == FLyraAbilitySet_GameplayAbility::StaticStruct() ? Property : nullptr;
	}

	bool AddGrantedAbility(ULyraAbilitySet* Set, UClass* AbilityClass, const FGameplayTag& InputTag)
	{
		FArrayProperty* Property = Set ? GetGrantedAbilitiesProperty() : nullptr;
		FStructProperty* StructProperty = Property ? CastField<FStructProperty>(Property->Inner) : nullptr;
		if (!StructProperty) { return false; }
		FScriptArrayHelper Helper(Property, Property->ContainerPtrToValuePtr<void>(Set));
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			const FLyraAbilitySet_GameplayAbility* Entry = reinterpret_cast<const FLyraAbilitySet_GameplayAbility*>(Helper.GetRawPtr(Index));
			if (Entry && Entry->Ability.Get() == AbilityClass && Entry->InputTag == InputTag) { return true; }
		}
		FLyraAbilitySet_GameplayAbility Entry;
		Entry.Ability = AbilityClass;
		Entry.AbilityLevel = 1;
		Entry.InputTag = InputTag;
		const int32 Index = Helper.AddValue();
		StructProperty->CopyCompleteValue(Helper.GetRawPtr(Index), &Entry);
		Set->MarkPackageDirty();
		return true;
	}

	int32 CountGrantedAbility(const ULyraAbilitySet* Set, UClass* AbilityClass, const FGameplayTag& InputTag)
	{
		FArrayProperty* Property = Set ? GetGrantedAbilitiesProperty() : nullptr;
		if (!Property) { return 0; }
		FScriptArrayHelper Helper(Property, Property->ContainerPtrToValuePtr<void>(const_cast<ULyraAbilitySet*>(Set)));
		int32 Count = 0;
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			const FLyraAbilitySet_GameplayAbility* Entry = reinterpret_cast<const FLyraAbilitySet_GameplayAbility*>(Helper.GetRawPtr(Index));
			if (Entry && Entry->Ability.Get() == AbilityClass && Entry->InputTag == InputTag) { ++Count; }
		}
		return Count;
	}

	void EnsureAbilityInput(ULyraInputConfig* Config, const UInputAction* Action, const FGameplayTag& InputTag)
	{
		if (!Config || !Action) { return; }
		for (FLyraInputAction& Entry : Config->AbilityInputActions)
		{
			if (!Entry.InputAction || Entry.InputAction->GetPathName() != Action->GetPathName()) { continue; }
			bool bChanged = false;
			if (Entry.InputAction != Action) { Entry.InputAction = Action; bChanged = true; }
			if (Entry.InputTag != InputTag) { Entry.InputTag = InputTag; bChanged = true; }
			if (bChanged) { Config->MarkPackageDirty(); }
			return;
		}
		FLyraInputAction& Entry = Config->AbilityInputActions.AddDefaulted_GetRef();
		Entry.InputAction = Action;
		Entry.InputTag = InputTag;
		Config->MarkPackageDirty();
	}

	ULyraInputConfig* CreateOrLoadInputConfig(const UInputAction* OpenRewards, const UInputAction* Wave)
	{
		ULyraInputConfig* Config = CreateOrLoadDataAsset<ULyraInputConfig>(InputConfigPath, ULyraInputConfig::StaticClass());
		EnsureAbilityInput(Config, OpenRewards, TAG_MonsterArena_Input_OpenRewards);
		EnsureAbilityInput(Config, Wave, TAG_MonsterArena_Input_WaveAction);
		return Config;
	}

	ULyraAbilitySet* CreateOrLoadAbilitySet()
	{
		ULyraAbilitySet* Set = CreateOrLoadDataAsset<ULyraAbilitySet>(AbilitySetPath, ULyraAbilitySet::StaticClass());
		if (Set)
		{
			AddGrantedAbility(Set, UMonsterArenaOpenRewardsAbility::StaticClass(), TAG_MonsterArena_Input_OpenRewards);
			AddGrantedAbility(Set, UMonsterArenaWaveActionAbility::StaticClass(), TAG_MonsterArena_Input_WaveAction);
		}
		return Set;
	}

	ULyraExperienceActionSet* LoadActionSet()
	{
		return LoadAssetIfPresent<ULyraExperienceActionSet>(ActionSetPath);
	}

	bool ConfigureActionSet(ULyraExperienceActionSet* ActionSet, UInputMappingContext* Context, const ULyraInputConfig* InputConfig)
	{
		if (!ActionSet || !Context || !InputConfig) { return false; }
		UGameFeatureAction_AddInputContextMapping* ContextAction = nullptr;
		UGameFeatureAction_AddInputBinding* BindingAction = nullptr;
		for (UObject* Action : ActionSet->Actions)
		{
			if (!ContextAction) { ContextAction = Cast<UGameFeatureAction_AddInputContextMapping>(Action); }
			if (!BindingAction) { BindingAction = Cast<UGameFeatureAction_AddInputBinding>(Action); }
		}
		if (!ContextAction)
		{
			ContextAction = NewObject<UGameFeatureAction_AddInputContextMapping>(ActionSet, NAME_None, RF_Transactional);
			ActionSet->Actions.Add(ContextAction);
			ActionSet->MarkPackageDirty();
		}
		const bool bHasContext = ContextAction->InputMappings.ContainsByPredicate([Context](const FInputMappingContextAndPriority& Entry)
		{
			return Entry.InputMapping.LoadSynchronous() == Context;
		});
		if (!bHasContext)
		{
			FInputMappingContextAndPriority& Entry = ContextAction->InputMappings.AddDefaulted_GetRef();
			Entry.InputMapping = Context;
			Entry.Priority = 0;
			Entry.bRegisterWithSettings = true;
			ActionSet->MarkPackageDirty();
		}
		if (!BindingAction)
		{
			BindingAction = NewObject<UGameFeatureAction_AddInputBinding>(ActionSet, NAME_None, RF_Transactional);
			ActionSet->Actions.Add(BindingAction);
			ActionSet->MarkPackageDirty();
		}
		const bool bHasBinding = BindingAction->InputConfigs.ContainsByPredicate([InputConfig](const TSoftObjectPtr<const ULyraInputConfig>& Entry)
		{
			return Entry.LoadSynchronous() == InputConfig;
		});
		if (!bHasBinding)
		{
			BindingAction->InputConfigs.Add(InputConfig);
			ActionSet->MarkPackageDirty();
		}
		return true;
	}

	UBlueprint* LoadExperienceBlueprint()
	{
		return LoadAssetIfPresent<UBlueprint>(ExperienceBlueprintPath);
	}

	ULyraExperienceDefinition* GetExperienceDefaults(UBlueprint* Blueprint)
	{
		return Blueprint && Blueprint->GeneratedClass ? Cast<ULyraExperienceDefinition>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr;
	}

	bool ConfigureExperienceGrant(ULyraExperienceDefinition* Experience, ULyraAbilitySet* AbilitySet)
	{
		if (!Experience || !AbilitySet) { return false; }
		UGameFeatureAction_AddAbilities* AbilitiesAction = nullptr;
		for (UObject* Action : Experience->Actions)
		{
			AbilitiesAction = Cast<UGameFeatureAction_AddAbilities>(Action);
			if (AbilitiesAction) { break; }
		}
		if (!AbilitiesAction)
		{
			AbilitiesAction = NewObject<UGameFeatureAction_AddAbilities>(Experience, NAME_None, RF_Transactional);
			Experience->Actions.Add(AbilitiesAction);
			Experience->MarkPackageDirty();
		}
		FGameFeatureAbilitiesEntry* Target = nullptr;
		for (FGameFeatureAbilitiesEntry& Entry : AbilitiesAction->AbilitiesList)
		{
			if (Entry.ActorClass.LoadSynchronous() == ALyraPlayerState::StaticClass()) { Target = &Entry; break; }
		}
		if (!Target)
		{
			Target = &AbilitiesAction->AbilitiesList.AddDefaulted_GetRef();
			Target->ActorClass = ALyraPlayerState::StaticClass();
		}
		const bool bExists = Target->GrantedAbilitySets.ContainsByPredicate([AbilitySet](const TSoftObjectPtr<const ULyraAbilitySet>& Entry)
		{
			return Entry.LoadSynchronous() == AbilitySet;
		});
		if (!bExists)
		{
			Target->GrantedAbilitySets.Add(AbilitySet);
			Experience->MarkPackageDirty();
		}
		return true;
	}

	int32 CountExperienceGrants(const ULyraExperienceDefinition* Experience, const ULyraAbilitySet* AbilitySet)
	{
		int32 Count = 0;
		if (!Experience || !AbilitySet) { return Count; }
		for (const UObject* Action : Experience->Actions)
		{
			const UGameFeatureAction_AddAbilities* AbilitiesAction = Cast<UGameFeatureAction_AddAbilities>(Action);
			if (!AbilitiesAction) { continue; }
			for (const FGameFeatureAbilitiesEntry& Entry : AbilitiesAction->AbilitiesList)
			{
				if (Entry.ActorClass.LoadSynchronous() != ALyraPlayerState::StaticClass()) { continue; }
				for (const TSoftObjectPtr<const ULyraAbilitySet>& Granted : Entry.GrantedAbilitySets)
				{
					if (Granted.LoadSynchronous() == AbilitySet) { ++Count; }
				}
			}
		}
		return Count;
	}

	bool HasSingleActionSetInput(const ULyraExperienceActionSet* ActionSet, const UInputMappingContext* Context, const ULyraInputConfig* InputConfig)
	{
		if (!ActionSet) { return false; }
		int32 ContextCount = 0;
		int32 BindingCount = 0;
		for (const UObject* Action : ActionSet->Actions)
		{
			if (const UGameFeatureAction_AddInputContextMapping* ContextAction = Cast<UGameFeatureAction_AddInputContextMapping>(Action))
			{
				for (const FInputMappingContextAndPriority& Entry : ContextAction->InputMappings)
				{
					if (Entry.InputMapping.LoadSynchronous() == Context) { ++ContextCount; }
				}
			}
			if (const UGameFeatureAction_AddInputBinding* BindingAction = Cast<UGameFeatureAction_AddInputBinding>(Action))
			{
				for (const TSoftObjectPtr<const ULyraInputConfig>& Entry : BindingAction->InputConfigs)
				{
					if (Entry.LoadSynchronous() == InputConfig) { ++BindingCount; }
				}
			}
		}
		return ContextCount == 1 && BindingCount == 1;
	}
}

UInputMappingContext* UMonsterArenaInputLibrary::CreateInputAssets()
{
	UInputAction* OpenRewards = CreateOrLoadAction(OpenRewardsActionPath, UMonsterArenaRewardInputAction::StaticClass());
	UInputAction* Wave = CreateOrLoadAction(WaveActionPath, UInputAction::StaticClass());
	UInputMappingContext* Context = CreateOrLoadMappingContext();
	if (!OpenRewards || !Wave || !Context) { return nullptr; }
	EnsurePlayerMappableSettings(OpenRewards, TEXT("MonsterArena.OpenRewards"), LOCTEXT("RewardActionMapping", "打开强化选择"), LOCTEXT("InputCategory", "怪物竞技场"));
	EnsurePlayerMappableSettings(Wave, TEXT("MonsterArena.WaveAction"), LOCTEXT("WaveActionMapping", "开始/下一波"), LOCTEXT("InputCategory", "怪物竞技场"));
	EnsureMapping(Context, OpenRewards, EKeys::U);
	EnsureMapping(Context, OpenRewards, EKeys::Gamepad_DPad_Up);
	EnsureMapping(Context, Wave, EKeys::N);
	EnsureMapping(Context, Wave, EKeys::Gamepad_DPad_Right);

	ULyraInputConfig* InputConfig = CreateOrLoadInputConfig(OpenRewards, Wave);
	ULyraAbilitySet* AbilitySet = CreateOrLoadAbilitySet();
	ULyraExperienceActionSet* ActionSet = LoadActionSet();
	if (!InputConfig || !AbilitySet || !ActionSet) { return nullptr; }
	ConfigureActionSet(ActionSet, Context, InputConfig);
	if (ULyraExperienceDefinition* Experience = GetExperienceDefaults(LoadExperienceBlueprint()))
	{
		ConfigureExperienceGrant(Experience, AbilitySet);
	}
	return Context;
}

bool UMonsterArenaInputLibrary::ValidateInputAssets()
{
	const UInputAction* OpenRewards = LoadAssetIfPresent<UInputAction>(OpenRewardsActionPath);
	const UInputAction* Wave = LoadAssetIfPresent<UInputAction>(WaveActionPath);
	const UInputMappingContext* Context = LoadAssetIfPresent<UInputMappingContext>(MappingContextPath);
	if (!OpenRewards || !Wave || !Context) { return false; }
	if (!OpenRewards->IsA<UMonsterArenaRewardInputAction>()) { return false; }
	if (OpenRewards->ValueType != EInputActionValueType::Boolean || Wave->ValueType != EInputActionValueType::Boolean) { return false; }
	const UPlayerMappableKeySettings* RewardKeySettings = OpenRewards->GetPlayerMappableKeySettings();
	const UPlayerMappableKeySettings* WaveKeySettings = Wave->GetPlayerMappableKeySettings();
	if (!RewardKeySettings || RewardKeySettings->GetMappingName() != FName(TEXT("MonsterArena.OpenRewards"))) { return false; }
	if (!WaveKeySettings || WaveKeySettings->GetMappingName() != FName(TEXT("MonsterArena.WaveAction"))) { return false; }
	if (!HasSingleMapping(Context, OpenRewards, EKeys::U)
		|| !HasSingleMapping(Context, OpenRewards, EKeys::Gamepad_DPad_Up)
		|| !HasSingleMapping(Context, Wave, EKeys::N)
		|| !HasSingleMapping(Context, Wave, EKeys::Gamepad_DPad_Right))
	{
		return false;
	}

	const ULyraInputConfig* InputConfig = LoadAssetIfPresent<ULyraInputConfig>(InputConfigPath);
	if (!InputConfig) { return false; }
	int32 InputConfigCount = 0;
	int32 WaveInputConfigCount = 0;
	for (const FLyraInputAction& Entry : InputConfig->AbilityInputActions)
	{
		if (Entry.InputAction == OpenRewards && Entry.InputTag == TAG_MonsterArena_Input_OpenRewards) { ++InputConfigCount; }
		if (Entry.InputAction == Wave && Entry.InputTag == TAG_MonsterArena_Input_WaveAction) { ++WaveInputConfigCount; }
	}
	if (InputConfigCount != 1 || WaveInputConfigCount != 1) { return false; }

	const ULyraAbilitySet* AbilitySet = LoadAssetIfPresent<ULyraAbilitySet>(AbilitySetPath);
	if (!AbilitySet
		|| CountGrantedAbility(AbilitySet, UMonsterArenaOpenRewardsAbility::StaticClass(), TAG_MonsterArena_Input_OpenRewards) != 1
		|| CountGrantedAbility(AbilitySet, UMonsterArenaWaveActionAbility::StaticClass(), TAG_MonsterArena_Input_WaveAction) != 1)
	{
		return false;
	}

	const ULyraExperienceActionSet* ActionSet = LoadActionSet();
	if (!ActionSet || !HasSingleActionSetInput(ActionSet, Context, InputConfig)) { return false; }

	const UBlueprint* ExperienceBlueprint = LoadExperienceBlueprint();
	const ULyraExperienceDefinition* Experience = ExperienceBlueprint && ExperienceBlueprint->GeneratedClass ? Cast<ULyraExperienceDefinition>(ExperienceBlueprint->GeneratedClass->GetDefaultObject()) : nullptr;
	return CountExperienceGrants(Experience, AbilitySet) == 1;
}

#undef LOCTEXT_NAMESPACE
