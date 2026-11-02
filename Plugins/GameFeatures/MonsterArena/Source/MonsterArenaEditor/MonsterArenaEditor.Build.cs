using UnrealBuildTool;

public class MonsterArenaEditor : ModuleRules
{
	public MonsterArenaEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] { "MonsterArenaRuntime", "LyraGame", "GameFeatures", "GameplayAbilities", "GameplayTags", "GameplayMessageRuntime", "UnrealEd", "UMG", "UMGEditor", "AssetTools", "AssetRegistry", "StateTreeModule", "StateTreeEditorModule", "GameplayStateTreeModule", "PropertyBindingUtils" });
	}
}
