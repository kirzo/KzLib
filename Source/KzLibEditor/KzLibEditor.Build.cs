// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzLibEditor : ModuleRules
{
	public KzLibEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"KzLib",
				"ComponentVisualizers"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"PropertyEditor",
				"BlueprintGraph",
				"KismetWidgets"
			});
	}
}