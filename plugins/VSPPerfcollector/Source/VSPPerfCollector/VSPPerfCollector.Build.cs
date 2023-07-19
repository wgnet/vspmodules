
using UnrealBuildTool;

public class VSPPerfCollector : ModuleRules
{
	public VSPPerfCollector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;
		CppStandard = CppStandardVersion.Cpp17;	

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"TraceServices",
				"CoreUObject",
				"Json",
				"JsonUtilities",
				"TraceLog",
				"TraceAnalysis"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"HTTP",
				"Cbor", 
				"VSPCommonUtils",
			}
		);

		bBuildLocallyWithSNDBS = true;
	}
}
