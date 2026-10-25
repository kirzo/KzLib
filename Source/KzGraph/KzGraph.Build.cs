// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzGraph : ModuleRules
{
	public KzGraph(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"SlateCore",
				"KzLib"
			}
			);
	}
}