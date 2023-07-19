using UnrealBuildTool;

public class FlowMapPainterEditor : ModuleRules
{
	public FlowMapPainterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"InteractiveToolsFramework",
				"EditorInteractiveToolsFramework",
				"EditorStyle",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"FluidSurfaces"
			}
		);
	}
}
