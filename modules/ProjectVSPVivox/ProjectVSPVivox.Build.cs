using UnrealBuildTool;

public class ProjectVSPVivox : ModuleRules
{
	public ProjectVSPVivox(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[]
			{
				"ProjectVSPVivox",
			});
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"VivoxCore",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"VSPCommonUtils",
				"TraceLog",
			});

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
				});
		}
	}
}
