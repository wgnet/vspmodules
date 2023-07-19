using UnrealBuildTool;

public class VSPPerfViewer : ModuleRules
{
    public VSPPerfViewer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	    PrecompileForTargets = PrecompileTargetsType.Any;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "SlateCore",
                "Slate",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "InputCore",
                "VSPPerfCollector",
                "UMG",
            }
        );

        if (Target.bBuildEditor)
        {
	        PublicDependencyModuleNames.AddRange(
		        new string[]
		        {
			        "UnrealEd",
		        });
        }

        bBuildLocallyWithSNDBS = true;
    }
}