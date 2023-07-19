
using System.IO;
using UnrealBuildTool;

public class TerrainGenerator : ModuleRules
{
	public TerrainGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[]
			{
				//Path.Combine( ModuleDirectory, "Public"),
				//Path.Combine(PluginDirectory, "Source/TerrainGenerator/Public"),
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				//Path.Combine( ModuleDirectory, "Private"),
				//Path.Combine(PluginDirectory, "Source/TerrainGenerator/Private"),
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"RenderCore", 
				"RHI",
				"Foliage",
				"Landscape",
				"LandscapeEditor",
				"LandscapeEditorUtilities",
				"UnrealEd",
				"MaterialBaking",
				"MeshMergeUtilities",
				"MeshDescription",
				"StaticMeshDescription",
				"Blutility",
			}
			);
	}
}
