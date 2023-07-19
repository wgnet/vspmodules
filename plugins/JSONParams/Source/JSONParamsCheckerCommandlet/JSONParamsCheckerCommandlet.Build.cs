using UnrealBuildTool;

public class JSONParamsCheckerCommandlet : ModuleRules
{
	public JSONParamsCheckerCommandlet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"JSONParams",
			});
	}
}
