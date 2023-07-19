
using UnrealBuildTool;

public class AtlasBaker : ModuleRules
{
	public AtlasBaker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"AtlasRuntime",
				"RenderCore"
			});
	}
}
