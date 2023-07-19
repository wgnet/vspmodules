using UnrealBuildTool;

public class ContentInspectorCommandlet : ModuleRules
{
	public ContentInspectorCommandlet(ReadOnlyTargetRules Target) : base(Target)
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
				"MessageLog",
				"ContentInspector",
				"Json",
				"JsonUtilities",
				"HTTP",
			});
	}
}
