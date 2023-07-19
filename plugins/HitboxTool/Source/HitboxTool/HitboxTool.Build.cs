
using UnrealBuildTool;

public class HitboxTool : ModuleRules
{
	public HitboxTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"UnrealEd",
				"Persona", 
				"SkeletalMeshEditor",
				"ProceduralMeshComponent",
				"GeometricObjects",
				"AdvancedPreviewScene",
				"Kismet",
			}
			);
	}
}
