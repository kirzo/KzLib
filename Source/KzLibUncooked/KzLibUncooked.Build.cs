// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzLibUncooked : ModuleRules
{
	public KzLibUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"KzLib"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"BlueprintGraph"
			});
	}
}