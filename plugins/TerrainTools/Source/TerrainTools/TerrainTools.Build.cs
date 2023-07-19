
using UnrealBuildTool;

public class TerrainTools : ModuleRules
{
	public TerrainTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Terraformer",
				"TerrainGenerator"
			}
		);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"LevelEditor",
				"Landscape",
				"LandscapeEditor",
				"RenderCore", 
				"RHI",
				"Foliage",
				"LandscapeEditorUtilities",
				"UnrealEd",
				"PlacementMode",
				"Slate",
				"SlateCore",
				"Projects"
			}
		);
	}
}
