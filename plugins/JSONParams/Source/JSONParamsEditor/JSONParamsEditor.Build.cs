using UnrealBuildTool;

public class JSONParamsEditor : ModuleRules
{
	public JSONParamsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;	

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"JSONParams",
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"EditorStyle",
				"DesktopPlatform",
				"EditorWidgets",
				"ContentBrowserData",
				"BlueprintGraph",
				"Kismet",
			});
	}
}
