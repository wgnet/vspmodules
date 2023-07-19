
using UnrealBuildTool;

public class StringTablesChecker : ModuleRules
{
	public StringTablesChecker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
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
				"Slate",
				"SlateCore",
				"UnrealEd",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				"VSPCommonUtils",
				"TraceLog",
			});
	}
}
