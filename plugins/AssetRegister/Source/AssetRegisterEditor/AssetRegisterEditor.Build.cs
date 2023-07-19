using UnrealBuildTool;

public class AssetRegisterEditor : ModuleRules
{
	public AssetRegisterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"EditorSubsystem",
				"EditorStyle",
				"Engine",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AssetRegisterRuntime",
				"GameplayTags",
				"DeveloperSettings",
				"Blutility",
				"ContentBrowser",
				"AssetManagerEditor",
			});
	}
}
