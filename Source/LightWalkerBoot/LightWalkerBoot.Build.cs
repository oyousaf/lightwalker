using UnrealBuildTool;

public class LightWalkerBoot : ModuleRules
{
	public LightWalkerBoot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "MoviePlayer", "Slate", "SlateCore" });
	}
}
