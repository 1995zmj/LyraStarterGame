using UnrealBuildTool;

public class MonsterArenaRuntime : ModuleRules
{
	public MonsterArenaRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "LyraGame", "GameplayAbilities", "GameplayTags", "GameplayTasks", "ModularGameplay", "AIModule", "UMG" });
		PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "NetCore" });
	}
}
