using UnrealBuildTool;

public class VSPAsync : ModuleRules
{
	public VSPAsync(ReadOnlyTargetRules Target) : base(Target)
	{
        CppStandard = CppStandardVersion.Cpp17;

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine", 
				// "VSPTests",
				"VSPCommonUtils",
				"TraceLog",
            });
	}
}
