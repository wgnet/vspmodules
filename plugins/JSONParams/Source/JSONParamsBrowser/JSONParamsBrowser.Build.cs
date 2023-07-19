using UnrealBuildTool;

public class JSONParamsBrowser : ModuleRules
{
    public JSONParamsBrowser(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;	

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "ContentBrowserData"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "JSONParams",
                "JSONParamsEditor",
            }
        );
    }
}