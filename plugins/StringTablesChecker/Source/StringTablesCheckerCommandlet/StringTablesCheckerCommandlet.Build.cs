using UnrealBuildTool;

public class StringTablesCheckerCommandlet : ModuleRules
{
	public StringTablesCheckerCommandlet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"StringTablesChecker",
			});
	}
}
