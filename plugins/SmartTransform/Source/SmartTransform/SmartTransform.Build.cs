using UnrealBuildTool;

public class SmartTransform: ModuleRules
{
	public SmartTransform(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Projects",
				"AppFramework",
				"Engine",
				"Slate",
				"ApplicationCore",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"EditorStyle",
				"ViewportSnapping",
				"TraceLog",
			});
	}
}
