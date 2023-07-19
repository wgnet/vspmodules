
using UnrealBuildTool;

public class VSPColorSchemes : ModuleRules
{
	public VSPColorSchemes(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;


        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"VSPCommonUtils",
				"DeveloperSettings",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);
	}
}
