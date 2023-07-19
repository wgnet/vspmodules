
using UnrealBuildTool;

public class VSPFormat : ModuleRules
{
	public VSPFormat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"fmt",
				"Core",
				"VSPTests", 
				"CoreUObject",
			});
	}
}
