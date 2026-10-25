// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzGraphEditor : ModuleRules
{
	public KzGraphEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"GraphEditor",
				"KzLib",
				"KzLibEditor",
				"KzGraph"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"InputCore",
				"ToolMenus",
				"PropertyEditor",
				"ApplicationCore",
				"AdvancedPreviewScene"
			}
			);
	}
}