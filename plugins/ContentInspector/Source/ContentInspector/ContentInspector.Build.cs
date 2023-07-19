
using UnrealBuildTool;

public class ContentInspector : ModuleRules
{
	public ContentInspector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core",
				"CoreUObject",
				"Engine",
				"PropertyPath",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"ContentBrowser",
				"MessageLog",
				"Projects",
				"DeveloperSettings", 
				"EditorScriptingUtilities",
			}
		);
	}
}
