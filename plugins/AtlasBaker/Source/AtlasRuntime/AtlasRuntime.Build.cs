
using UnrealBuildTool;

public class AtlasRuntime : ModuleRules
{
	public AtlasRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
				"Slate",
				"SlateCore"
			});
	}
}
