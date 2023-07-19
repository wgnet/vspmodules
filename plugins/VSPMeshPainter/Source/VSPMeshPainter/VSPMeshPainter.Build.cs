using UnrealBuildTool;

public class VSPMeshPainter : ModuleRules
{
	public VSPMeshPainter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;	

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"MeshPaintingToolset",
				"MeshPaintEditorMode",
				"EditorStyle",
				"InteractiveToolsFramework",
				"PropertyEditor",
				"RHI",
				"RenderCore",
                "VSPCommonUtils",
				"VSPColorSchemes",
            }
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"DeveloperSettings",
				"Blutility",
				"UMG",
				"UMGEditor", 
				"EditorScriptingUtilities",
				"TraceLog",
			}
			);
	}
}
