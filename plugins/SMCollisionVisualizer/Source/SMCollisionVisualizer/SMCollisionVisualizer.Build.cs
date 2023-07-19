
using UnrealBuildTool;

public class SMCollisionVisualizer : ModuleRules
{
    public SMCollisionVisualizer(ReadOnlyTargetRules Target) : base(Target)
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
