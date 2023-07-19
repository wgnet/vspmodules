
using System.IO;
using UnrealBuildTool;

public class Terraformer : ModuleRules
{
	public Terraformer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"UnrealEd",
				"Landscape",
				"LandscapeEditorUtilities", 
				"DeveloperSettings",
				"Renderer",
				"RenderCore",
				"RHI",
				"Foliage",
				"ComponentVisualizers",
				"TerrainGenerator",
				"TerraformerRender"
			}
		);
	}
}
