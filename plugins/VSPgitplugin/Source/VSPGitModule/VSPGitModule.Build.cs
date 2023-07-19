
using UnrealBuildTool;

public class VSPGitModule : ModuleRules
{
	public VSPGitModule(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnforceIWYU = true;
		bLegacyPublicIncludePaths = false;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		if (Target.Type == TargetType.Program)
		{
			PublicIncludePaths.AddRange(new string[] {
				"Runtime/Launch/Public"
				}
			);
			PrivateIncludePaths.AddRange(new string[] {
				"Runtime/Launch/Private"		// For LaunchEngineLoop.cpp include
				}
			);

		}

        PrivateDependencyModuleNames.AddRange(
            new string[] {
	            "CoreUObject",
	            "ToolMenus",
                "AppFramework",
                "Core",
                "ApplicationCore",
                "EditorStyle",
                "Projects",
                "Slate",
                "SlateCore",
                "StandaloneRenderer",
                "SourceControl",
                "DesktopWidgets",
                "InputCore",
                "UnrealEd"
            }
        );

		if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.TVOS)
		{
			PrivateDependencyModuleNames.AddRange(
                new string [] {
                    "NetworkFile",
                    "StreamingFile"
                }
            );
		}

		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Linux))
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"UnixCommonStartup"
				}
			);
		}
	}
}
