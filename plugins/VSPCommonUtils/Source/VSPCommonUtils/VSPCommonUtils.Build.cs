using UnrealBuildTool;

public class VSPCommonUtils : ModuleRules
{
	public VSPCommonUtils(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"VSPFormat",
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"TraceLog",
				"GameplayTags",
				"VSPTests",
			});
	}
}
