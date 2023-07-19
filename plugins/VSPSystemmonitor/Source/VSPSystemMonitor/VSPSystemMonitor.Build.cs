using UnrealBuildTool;

public class VSPSystemMonitor : ModuleRules
{
	public VSPSystemMonitor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Projects",
					"Engine",
					"CoreUObject",
					"TraceLog",
				}
			);
		if (!Target.bBuildEditor)
		{
			switch (Target.Configuration)
			{
				case UnrealTargetConfiguration.DebugGame:
				case UnrealTargetConfiguration.Debug:
				case UnrealTargetConfiguration.Development:
					PublicDefinitions.Add("VSP_SYSTEM_MONITOR_DEBUG");
					break;
			}
		}
	}
}
